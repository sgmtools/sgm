#include "SGMGraphicsWidget.hpp"

#include <cmath>
#include <limits>
#include <QApplication>
#include <QMatrix4x4>
#include <QMenu>
#include <QMouseEvent>
#include <QOpenGLFunctions_3_2_Core>
#include <QWheelEvent>
#include <vector>

#include <QDebug>

// We are using the OpenGL 3.2 api
using QtOpenGL = QOpenGLFunctions_3_2_Core;

// Convenience function to get Qt's OpenGL interface.
static QtOpenGL* get_opengl()
{
  return QOpenGLContext::currentContext()->versionFunctions<QtOpenGL>();
}

class pShaders
{
private:
  GLuint mVertexShader;
  GLuint mFragmentShader;
  GLuint mShaderProgram;

  void create_vertex_shader(QtOpenGL* opengl)
  {
    const char* source =
        R"glsl(

        #version 150 core

        in vec3 position;
        in vec3 normal;
        in vec3 color;

        out vec3 vertex_normal;
        out vec3 vertex_color;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
          gl_Position = projection * view * model * vec4(position, 1.0);

          // Adjust the vertex normal based on the camera rotation
          vertex_normal = vec3(view*vec4(normal, 0.0));
          vertex_color = color;
        }

        )glsl";

    mVertexShader = opengl->glCreateShader(GL_VERTEX_SHADER);
    opengl->glShaderSource(mVertexShader, 1, &source, NULL);
    opengl->glCompileShader(mVertexShader);
  }

  void create_fragment_shader(QtOpenGL* opengl)
  {
    const char* source =
        R"glsl(

        #version 150 core

        in vec3 vertex_normal;
        in vec3 vertex_color;

        out vec4 outColor;

        uniform vec3 gColor;
        uniform vec3 light_direction;

        void main()
        {
          float ambient_strength = 0.6;
          vec3 light_dir_norm = normalize(light_direction);
          vec3 norm = normalize(vertex_normal);
          float diffuse_strength = 0.4*max(dot(norm, light_dir_norm), 0.0);

          outColor = vec4((ambient_strength + diffuse_strength)*vertex_color, 1.0);
        }

        )glsl";

    mFragmentShader = opengl->glCreateShader(GL_FRAGMENT_SHADER);
    opengl->glShaderSource(mFragmentShader, 1, &source, NULL);
    opengl->glCompileShader(mFragmentShader);
  }

public:
  pShaders() {}
  ~pShaders() {}

  void cleanup(QtOpenGL* opengl)
  {
    opengl->glDeleteProgram(mShaderProgram);
    opengl->glDeleteShader(mFragmentShader);
    opengl->glDeleteShader(mVertexShader);
  }

  void init_shaders(QtOpenGL* opengl)
  {
    // Setup the shader program
    create_vertex_shader(opengl);
    create_fragment_shader(opengl);
    mShaderProgram = opengl->glCreateProgram();
    opengl->glAttachShader(mShaderProgram, mVertexShader);
    opengl->glAttachShader(mShaderProgram, mFragmentShader);
    opengl->glLinkProgram(mShaderProgram);
    opengl->glUseProgram(mShaderProgram);
  }

  GLuint attribute(QtOpenGL* opengl, const GLchar* name)
  {
    return opengl->glGetAttribLocation(mShaderProgram, name);
  }

  GLuint uniform(QtOpenGL* opengl, const GLchar* name)
  {
    return opengl->glGetUniformLocation(mShaderProgram, name);
  }

  GLuint position_attribute(QtOpenGL* opengl)
  {
    return attribute(opengl, "position");
  }

  GLuint normal_attribute(QtOpenGL* opengl)
  {
    return attribute(opengl, "normal");
  }

  GLuint color_attribute(QtOpenGL* opengl)
  {
    return attribute(opengl, "color");
  }

  void set_color(QtOpenGL* opengl, float r, float g, float b)
  {
    GLuint var = uniform(opengl, "gColor");
    opengl->glUniform3f(var, r, g, b);
  }

  void set_model_transform(QtOpenGL* opengl, float* trans)
  {
    GLuint var = uniform(opengl, "model");
    opengl->glUniformMatrix4fv(var, 1, GL_FALSE, trans);
  }

  void set_view_transform(QtOpenGL* opengl, float* trans)
  {
    GLuint var = uniform(opengl, "view");
    opengl->glUniformMatrix4fv(var, 1, GL_FALSE, trans);
  }

  void set_projection_transform(QtOpenGL* opengl, float* trans)
  {
    GLuint var = uniform(opengl, "projection");
    opengl->glUniformMatrix4fv(var, 1, GL_FALSE, trans);
  }

  // Set the light direction (in camera coordinates)
  void set_light_direction(QtOpenGL* opengl, float x, float y, float z)
  {
    GLuint var = uniform(opengl, "light_direction");
    opengl->glUniform3f(var, x, y, z);
  }
};


class pCamera
{
private:
  bool mUpdateProjectionTransform;
  bool mUpdateViewTransform;
  bool mUpdateModelTransform;
  float xmin, xmax, ymin, ymax, zmin, zmax;
  float mZoomLevel;
  float mAspectRatio;
  bool mPerspective;
  QQuaternion mOrientation;
  QVector3D mTranslation;

  // Get the camera's x, y axes in worldspace (useful for
  // properly doing translations and rotations)
  void get_xy_axes(QVector3D &xaxis, QVector3D &yaxis)
  {
    QVector3D tmpx, tmpy, tmpz;
    mOrientation.getAxes(&tmpx, &tmpy, &tmpz);

    xaxis.setX(tmpx.x());
    xaxis.setY(tmpy.x());
    xaxis.setZ(tmpz.x());
    xaxis.normalize();

    yaxis.setX(tmpx.y());
    yaxis.setY(tmpy.y());
    yaxis.setZ(tmpz.y());
    yaxis.normalize();
  }

public:
  pCamera() :
    mUpdateProjectionTransform(true),
    mUpdateViewTransform(true),
    mUpdateModelTransform(true),
    mPerspective(true)
  {
    reset_model_transform();
    reset_view();
  }

  ~pCamera() {}

  void reset_model_transform()
  {
    xmin = ymin = zmin = std::numeric_limits<float>::max();
    xmax = ymax = zmax = std::numeric_limits<float>::lowest();

    mUpdateModelTransform = true;
  }

  void reset_bounds()
  {
    xmin = ymin = zmin = std::numeric_limits<float>::max();
    xmax = ymax = zmax = std::numeric_limits<float>::lowest();

    mUpdateModelTransform = true;
  }

  void reset_view()
  {
    mOrientation = QQuaternion::fromDirection(QVector3D(0.0, 0.0, 1.0),
                                              QVector3D(0.0, 1.0, 0.0));

    mTranslation = QVector3D(0.0, 0.0, 0.0);
    mZoomLevel = 0.5;

    mUpdateViewTransform = true;
    mUpdateProjectionTransform = true;
  }

  void enable_perspective(bool enable)
  {
    mPerspective = enable;
    mUpdateProjectionTransform = true;
  }

  void set_viewport_size(int width, int height)
  {
    mAspectRatio = (float) width / (float) height;
    mUpdateProjectionTransform = true;
  }

  void set_translation(int x, int y)
  {
    float deltax = (float) x / 100.0;
    float deltay = (float) -y / 100.0;

    QVector3D xaxis, yaxis;
    get_xy_axes(xaxis, yaxis);

    mTranslation += xaxis*deltax + yaxis*deltay;
    mUpdateViewTransform = true;
  }

  void set_rotation(int x, int y)
  {
    float xangle = (float) x / 2.5;
    float yangle = (float) y / 2.5;

    QVector3D xaxis, yaxis;
    get_xy_axes(xaxis, yaxis);

    QQuaternion rotation =
        QQuaternion::fromAxisAndAngle(yaxis, xangle)*
        QQuaternion::fromAxisAndAngle(xaxis, yangle);

    mOrientation *= rotation;
    mUpdateViewTransform = true;
  }

  // Set the absolute zoom factor. A zoom factor of 0 indicates the camera
  // is zoomed out as far as it can go. A zoom factor of 100 indicates the
  // camera is at its maximum zoom.
  void set_zoom(uint factor)
  {
    mZoomLevel = (float) factor / 100.0;

    // Clamp the zoom level
    if(mZoomLevel > 1.0)
      mZoomLevel = 1.0;

    mUpdateProjectionTransform = true;
  }

  // A negative factor indicates zoom out. Positive indicates zoom in.
  void increment_zoom(float increment)
  {
    mZoomLevel += increment;

    // Clamp the zoom level;
    if(mZoomLevel > 1.0)
      mZoomLevel = 1.0;
    else if(mZoomLevel < 0.0)
      mZoomLevel = 0.0;

    mUpdateProjectionTransform = true;
  }

void update_point_bounds(float x, float y, float z)
  {
    if(x > xmax)
      xmax = x;
    if(x < xmin)
      xmin = x;

    if(y > ymax)
      ymax = y;
    if(y < ymin)
      ymin = y;

    if(z > zmax)
      zmax = z;
    if(z < zmin)
      zmin = z;

    mUpdateModelTransform = true;
  }

  QMatrix4x4 projection_transform()
  {
    QMatrix4x4 projection;

    // If we did our model scaling correctly, things should be bounded between -1 and 1
    // for all axes. This means our left and right bounds will also be -1 and 1. To allow
    // users to zoom out a little beyond that, we will put view bounds between -2 and 2.
    float max_half_height = 2.0f;
    float min_half_eight = 0.001f;

    if(mPerspective)
    {
      // Calculate max angle. Camera is 5 from the origin (see view_transform)
      const float r_to_d = 57.2958f;
      float max_half_angle = atan(max_half_height/5.0)*r_to_d;
      float min_half_angle = atan(min_half_eight/5.0)*r_to_d;
      float vertical_field_of_view = min_half_angle +
          (max_half_angle - min_half_angle)*(1.0 - mZoomLevel);

      projection.perspective(2.0*vertical_field_of_view,
                             mAspectRatio,
                             0.1f,    // near plane
                             100.0f); // far plane
    }
    else
    {
      float half_height = min_half_eight +
          (max_half_height - min_half_eight) * mZoomLevel;
      float half_width = half_height*mAspectRatio;

      projection.ortho(-half_width,  //left
                       half_width,   //right
                       -half_height, // bottom
                       half_height,  //top
                       0.1f,         // near plane
                       100.0f);      // far plane
    }

    return projection;
  }

  QMatrix4x4 view_transform()
  {
    QMatrix4x4 view;

    // Move the camera back from the center of the model so that we can actually see it
    view.translate(0.0, 0.0, -5.0);

    // Apply camera rotations and translations
    view.rotate(mOrientation);
    view.translate(mTranslation);

    return view;
  }

  QMatrix4x4 model_transform()
  {
    QMatrix4x4 model;

    // Determine an appropriate scale factor for the model so that
    // it is always rendered between -1.0 and 1.0 on the GPU.
    float scale = 1.0;
    float xdiff = (xmax - xmin);
    float ydiff = (ymax - ymin);
    float zdiff = (zmax - zmin);

    if(xdiff > ydiff)
    {
      if(xdiff > zdiff)
        scale = 2.0/xdiff;
      else
        scale = 2.0/zdiff;
    }
    else if(ydiff > zdiff)
    {
      scale = 2.0/ydiff;
    }
    else
    {
      scale = 2.0/zdiff;
    }

    if(scale < 0.0)
      scale*= -1.0;

    // Calculate the translation necessary to center the model at 0, 0, 0
    float xavg = (xmax + xmin)/(2.0);
    float yavg = (ymax + ymin)/(2.0);
    float zavg = (zmax + zmin)/(2.0);

    // Scale and center the model
    model.scale(scale);
    model.translate(-xavg, -yavg, -zavg);

    return model;
  }

  void update_transforms(QtOpenGL* opengl, pShaders* shaders)
  {
    if(mUpdateModelTransform)
    {
      QMatrix4x4 model = model_transform();
      shaders->set_model_transform(opengl, model.data());
      mUpdateModelTransform = false;
    }

    if(mUpdateViewTransform)
    {
      QMatrix4x4 view = view_transform();
      shaders->set_view_transform(opengl, view.data());
      mUpdateViewTransform = false;
    }

    if(mUpdateProjectionTransform)
    {
      QMatrix4x4 projection = projection_transform();
      shaders->set_projection_transform(opengl, projection.data());
      mUpdateProjectionTransform = false;
    }
  }

};


struct pMouseData
{
  enum MouseFunction
  {
    NONE,
    ZOOM,
    PAN,
    ROTATE
  };

  QPoint press_pos;
  QPoint last_pos;
  MouseFunction function;
  bool is_drag;

  pMouseData() :
    press_pos(0, 0),
    last_pos(0, 0),
    function(NONE),
    is_drag(false)
  {}

  void reset()
  {
    press_pos = last_pos = QPoint(0,0);
    function = NONE;
    is_drag = false;
  }
};


class pOpenGLFloatData
{
private:
  GLuint  mArray;          // Variable to store shader settings
  GLuint  mVertexBuffer;   // Variable to store vertex buffer object (points, normals, etc.)
  GLuint  mElementBuffer;  // Variable to store element array object (e.g., triangle indices)
  GLsizei mNumElements;    // Number of indices to render in the element buffer
  GLenum  mRenderMode;     // How to render the data (e.g., GL_TRIANGLES)

  GLfloat mRGB[3];  // Color indices

  std::vector<GLfloat> mTempDataBuffer;
  std::vector<GLuint>  mTempIndexBuffer;

public:
  pOpenGLFloatData() {}
  ~pOpenGLFloatData() {}

  void cleanup(QtOpenGL* opengl)
  {
    opengl->glDeleteBuffers(1, &mElementBuffer);
    opengl->glDeleteBuffers(1, &mVertexBuffer);
    opengl->glDeleteVertexArrays(1, &mArray);
  }

  void init(QtOpenGL* opengl)
  {
    mNumElements = 0;

    // Create variables in OpenGL for the vertex array, vertex buffer object, and element
    // buffer object
    opengl->glGenVertexArrays(1, &mArray);
    opengl->glGenBuffers(1, &mVertexBuffer);
    opengl->glGenBuffers(1, &mElementBuffer);

    // Bind the vertex array first to save buffer settings
    opengl->glBindVertexArray(mArray);

    // Bind the vertex buffer so that future calls to glVertexAttribPointer are tied
    // to the correct buffer.
    opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

    // Bind the element buffer so that the vertex array will remember it.
    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
  }

  // Define data layout for the buffers. This function should be called immediately
  // after init()
  void set_float_attribute(QtOpenGL* opengl,
                           GLuint attribute, GLint size, GLint stride, GLint offset)
  {
    // Enable the attribute
    opengl->glEnableVertexAttribArray(attribute);

    // Set the memory layout information
    opengl->glVertexAttribPointer(
          attribute,
          size,   // Set the size for the attribute (i.e., how many indices in the vertex
                  // buffer are used for this one attribute)
          GL_FLOAT, // data type is float
          GL_FALSE, // Don't normalize the data
          stride*sizeof(GLfloat), // Set the stride for the vertex buffer
          (void*) (offset*sizeof(GLfloat)) // Define the offset where the data begins
                                           // in the vertex buffer.
          );
  }

  void set_color(GLfloat r, GLfloat g, GLfloat b)
  {
    mRGB[0] = r;
    mRGB[1] = g;
    mRGB[2] = b;
  }

  void set_render_mode(GLenum mode)
  {
    mRenderMode = mode;
  }

  void render(QtOpenGL* opengl, pShaders* shaders)
  {
    if(mNumElements == 0)
      return;

    // Update the color
    shaders->set_color(opengl, mRGB[0], mRGB[1], mRGB[2]);

    // Make sure our buffers are bound and ready to be rendered
    opengl->glBindVertexArray(mArray);
    opengl->glDrawElements(
          mRenderMode,
          mNumElements,    // Number of items in the element buffer to render
          GL_UNSIGNED_INT, // Element buffer type
          0                // No offset
          );
  }

  std::vector<GLfloat>& temp_data_buffer()
  {
    return mTempDataBuffer;
  }

  std::vector<GLuint>& temp_index_buffer()
  {
    return mTempIndexBuffer;
  }

  void flush(QtOpenGL* opengl)
  {
    // This code can cause edges to stay around after they have been deleted.
    //if(mTempDataBuffer.empty() || mTempIndexBuffer.empty())
    //  return;

    // Make our buffers active
    opengl->glBindVertexArray(mArray);
    opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);

    // Update the data in the buffers.
    opengl->glBufferData(GL_ARRAY_BUFFER,
                         mTempDataBuffer.size()*sizeof(GLfloat),
                         mTempDataBuffer.data(),
                         GL_STATIC_DRAW);

    opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         mTempIndexBuffer.size()*sizeof(GLuint),
                         mTempIndexBuffer.data(),
                         GL_STATIC_DRAW);

    // Empty the temp buffers
    mNumElements = (GLsizei)mTempIndexBuffer.size();
    mTempDataBuffer.clear();
    mTempIndexBuffer.clear();
  }

};

struct pGraphicsData
{
  pShaders shaders;
  pCamera camera;
  pMouseData mouse;

  pOpenGLFloatData vertex_data;
  pOpenGLFloatData edge_data;
  pOpenGLFloatData face_data;

  bool render_faces;
  bool render_edges;
  bool render_vertices;
  bool render_facets;
  bool render_uvspace;
  bool render_uv;
};

SGMGraphicsWidget::SGMGraphicsWidget(QWidget *parent, Qt::WindowFlags f) :
  QOpenGLWidget(parent, f),
  dPtr(new pGraphicsData)
{
  dPtr->render_faces = true;
}

SGMGraphicsWidget::~SGMGraphicsWidget()
{
  QtOpenGL* opengl = get_opengl();
  if(opengl)
  {
    dPtr->shaders.cleanup(opengl);
    dPtr->vertex_data.cleanup(opengl);
    dPtr->edge_data.cleanup(opengl);
    dPtr->face_data.cleanup(opengl);
  }

  delete dPtr;
}

void SGMGraphicsWidget::reset_bounds()
    {
    dPtr->camera.reset_bounds();
    }

void SGMGraphicsWidget::add_face(const std::vector<SGM::Point3D>      &points,
                                 const std::vector<unsigned int>      &triangles,
                                 const std::vector<SGM::UnitVector3D> &norms,
                                 const std::vector<SGM::Vector3D>     &colors)
{
  std::vector<float> &data_buffer = dPtr->face_data.temp_data_buffer();
  std::vector<GLuint> &index_buffer = dPtr->face_data.temp_index_buffer();

  // Setup indices for the triangles
  size_t offset = data_buffer.size() / 9;
  for(size_t index : triangles)
    index_buffer.push_back((unsigned int)(offset+index));

  // I am assuming that norms and points will always be the same size.
  for(size_t i = 0; i < points.size(); i++)
  {
    // Add data to the buffer
    SGM::Point3D point = points[i];
    data_buffer.push_back(point.m_x);
    data_buffer.push_back(point.m_y);
    data_buffer.push_back(point.m_z);

    SGM::UnitVector3D normal = norms[i];
    data_buffer.push_back(normal.m_x);
    data_buffer.push_back(normal.m_y);
    data_buffer.push_back(normal.m_z);

    SGM::Vector3D color = colors[i];
    data_buffer.push_back(color.m_x);
    data_buffer.push_back(color.m_y);
    data_buffer.push_back(color.m_z);

    // Update the camera
    dPtr->camera.update_point_bounds(point.m_x, point.m_y, point.m_z);
  }
}

void SGMGraphicsWidget::add_edge(const std::vector<SGM::Point3D>  &points,
                                 const std::vector<SGM::Vector3D> &colors)
{
  if(points.empty())
    return;

  std::vector<GLfloat> &data_buffer = dPtr->edge_data.temp_data_buffer();
  std::vector<GLuint> &index_buffer = dPtr->edge_data.temp_index_buffer();

  // Setup indices for the line segments
  size_t offset = data_buffer.size() / 6;
  size_t num_points = points.size();
  for(size_t i = 0; i < num_points - 1; i++)
  {
    size_t index = offset+i;
    index_buffer.push_back((unsigned int)(index));
    index_buffer.push_back((unsigned int)(index+1));
  }

  // Add the point data
  size_t Index1;
  for(Index1=0;Index1<num_points;++Index1)
  {
    SGM::Point3D const &Pos=points[Index1];
    data_buffer.push_back(Pos.m_x);
    data_buffer.push_back(Pos.m_y);
    data_buffer.push_back(Pos.m_z);

    SGM::Vector3D const &color = colors[Index1];
    data_buffer.push_back(color.m_x);
    data_buffer.push_back(color.m_y);
    data_buffer.push_back(color.m_z);

    dPtr->camera.update_point_bounds(Pos.m_x, Pos.m_y, Pos.m_z);
  }
}

void SGMGraphicsWidget::add_vertex(SGM::Point3D  const &Pos,
                                   SGM::Vector3D const &ColorVec)
    {
    std::vector<GLfloat> &data_buffer = dPtr->vertex_data.temp_data_buffer();
    std::vector<GLuint> &index_buffer = dPtr->vertex_data.temp_index_buffer();

    // Setup indices for the point
    size_t offset = data_buffer.size() / 6;
    index_buffer.push_back((unsigned int)(offset));

    // Add the point data
    data_buffer.push_back(Pos.m_x);
    data_buffer.push_back(Pos.m_y);
    data_buffer.push_back(Pos.m_z);
        
    data_buffer.push_back(ColorVec.m_x);
    data_buffer.push_back(ColorVec.m_y);
    data_buffer.push_back(ColorVec.m_z);

    data_buffer.push_back(Pos.m_x);
    data_buffer.push_back(Pos.m_y);
    data_buffer.push_back(Pos.m_z+1);
        
    data_buffer.push_back(ColorVec.m_x);
    data_buffer.push_back(ColorVec.m_y);
    data_buffer.push_back(ColorVec.m_z);
        
    dPtr->camera.update_point_bounds(Pos.m_x, Pos.m_y, Pos.m_z);
    }

void SGMGraphicsWidget::flush()
{
  QtOpenGL* opengl = get_opengl();
  if(!opengl)
    return;

  dPtr->face_data.flush(opengl);
  dPtr->edge_data.flush(opengl);
  dPtr->vertex_data.flush(opengl);
  update();
}

void SGMGraphicsWidget::reset_view()
{
  dPtr->camera.reset_view();
  update();
}

void SGMGraphicsWidget::set_render_faces(bool render)
{
  dPtr->render_faces = render;
  update();
}

void SGMGraphicsWidget::set_render_edges(bool render)
{
  dPtr->render_edges = render;
  update();
}

void SGMGraphicsWidget::set_render_facets(bool render)
{
  dPtr->render_facets = render;
  update();
}

void SGMGraphicsWidget::set_render_uvspace(bool render)
{
  dPtr->render_uvspace = render;
  update();
}

void SGMGraphicsWidget::set_render_vertices(bool render)
{
  dPtr->render_vertices = render;
  update();
}

void SGMGraphicsWidget::enable_perspective(bool enable)
{
  dPtr->camera.enable_perspective(enable);
  update();
}

QSurfaceFormat SGMGraphicsWidget::default_format()
{
  QSurfaceFormat fmt;
  fmt.setRenderableType(QSurfaceFormat::OpenGL);
  fmt.setVersion(3, 2);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setRedBufferSize(1);
  fmt.setGreenBufferSize(1);
  fmt.setBlueBufferSize(1);
  fmt.setDepthBufferSize(1);
  fmt.setStencilBufferSize(0);
  fmt.setAlphaBufferSize(1);
  fmt.setStereo(false);
  fmt.setSwapInterval(0);

  return fmt;
}

void SGMGraphicsWidget::initializeGL()
{
  QtOpenGL* opengl = get_opengl();
  if(!opengl)
    return;

  opengl->initializeOpenGLFunctions();
  opengl->glEnable(GL_DEPTH_TEST);
  opengl->glEnable(GL_MULTISAMPLE);
  opengl->glEnable(GL_LINE_SMOOTH);
  opengl->glEnable(GL_BLEND);
  opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  opengl->glLineWidth(1.0);
  opengl->glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
  opengl->glClearColor(0.5, 0.5, 0.5, 1.0);

  opengl->glEnable(GL_POLYGON_OFFSET_FILL);
  opengl->glPolygonOffset(2,1);

  opengl->glEnable(GL_PROGRAM_POINT_SIZE);
  opengl->glPointSize(7);

  // Did not make round points.
  //opengl->glEnable(GL_POINT_SMOOTH);

  //GLfloat lineWidthRange[2] = {0.0f, 0.0f};
  //opengl->glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
  // Returned 0 to 7 for my laptop.  PRS
  //opengl->glLineWidth(5);

  dPtr->shaders.init_shaders(opengl);
  dPtr->shaders.set_light_direction(opengl, 0.0f, 0.1f, 1.0f);

  GLuint position_attribute = dPtr->shaders.position_attribute(opengl);
  GLuint normal_attribute = dPtr->shaders.normal_attribute(opengl);
  GLuint color_attribute = dPtr->shaders.color_attribute(opengl);

  // Setup vertex data
  dPtr->vertex_data.set_color(1.0, 0.0, 0.0);
  dPtr->vertex_data.set_render_mode(GL_POINTS);
  dPtr->vertex_data.init(opengl);
  dPtr->vertex_data.set_float_attribute(opengl, position_attribute, 3, 6, 0);
  dPtr->vertex_data.set_float_attribute(opengl, color_attribute, 3, 6, 3);

  // Setup edge data
  dPtr->edge_data.set_color(0.0, 0.0, 0.0);
  dPtr->edge_data.set_render_mode(GL_LINES);
  dPtr->edge_data.init(opengl);
  dPtr->edge_data.set_float_attribute(opengl, position_attribute, 3, 6, 0);
  dPtr->face_data.set_float_attribute(opengl, color_attribute, 3, 6, 3);

  // Setup face data
  dPtr->face_data.set_color(0.5, 0.5, 1.0);
  dPtr->face_data.set_render_mode(GL_TRIANGLES);
  dPtr->face_data.init(opengl);
  dPtr->face_data.set_float_attribute(opengl, position_attribute, 3, 9, 0);
  dPtr->face_data.set_float_attribute(opengl, normal_attribute, 3, 9, 3);
  dPtr->face_data.set_float_attribute(opengl, color_attribute, 3, 9, 6);
}

void SGMGraphicsWidget::paintGL()
{
  QtOpenGL* opengl = get_opengl();
  if(!opengl)
    return;

  qreal scale(devicePixelRatio());

  opengl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  opengl->glViewport(0, 0, this->width()*scale, this->height()*scale);

  // Apply transforms from the camera
  dPtr->camera.update_transforms(opengl, &dPtr->shaders);

  if(dPtr->render_faces)
    dPtr->face_data.render(opengl, &dPtr->shaders);

  if(dPtr->render_vertices)
    dPtr->vertex_data.render(opengl, &dPtr->shaders);

  if(dPtr->render_edges || dPtr->render_facets || dPtr->render_uvspace)
    dPtr->edge_data.render(opengl, &dPtr->shaders);
}

void SGMGraphicsWidget::resizeGL(int w, int h)
{
  // take care of HDPI screen, e.g. Retina display on Mac
  qreal scale(devicePixelRatio());

  // Set OpenGL viewport to cover whole widget
  dPtr->camera.set_viewport_size(w*scale, h*scale);
}

void SGMGraphicsWidget::mousePressEvent(QMouseEvent *event)
{
  dPtr->mouse.press_pos = event->pos();
  dPtr->mouse.last_pos = event->pos();
  if(event->button() == Qt::LeftButton)
    dPtr->mouse.function = pMouseData::ROTATE;
  else if(event->button() == Qt::MiddleButton)
    dPtr->mouse.function = pMouseData::ZOOM;
  else if(event->button() == Qt::RightButton)
    dPtr->mouse.function = pMouseData::PAN;
  else
    dPtr->mouse.function = pMouseData::NONE;
}

void SGMGraphicsWidget::mouseMoveEvent(QMouseEvent *event)
{
  // Check if we have moved far enough to consider it a drag
  if(!dPtr->mouse.is_drag)
  {
    int drag_distance = (event->pos() - dPtr->mouse.press_pos).manhattanLength();
    if(drag_distance > QApplication::startDragDistance())
      dPtr->mouse.is_drag = true;
  }
  else
  {
    QPoint diff = event->pos() - dPtr->mouse.last_pos;
    dPtr->mouse.last_pos = event->pos();

    if(dPtr->mouse.function == pMouseData::PAN)
      dPtr->camera.set_translation(diff.x(), diff.y());
    else if(dPtr->mouse.function == pMouseData::ROTATE)
      dPtr->camera.set_rotation(diff.x(), diff.y());
    else if(dPtr->mouse.function == pMouseData::ZOOM)
      dPtr->camera.increment_zoom(diff.y()*10);
    else
      return;

    update();
  }
}

void SGMGraphicsWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if(!dPtr->mouse.is_drag)
  {
    if(event->button() == Qt::RightButton)
      exec_context_menu(this->mapToGlobal(event->pos()));
  }

  dPtr->mouse.reset();
}

void SGMGraphicsWidget::wheelEvent(QWheelEvent *event)
{
  float increment = 0.02f;
  if(event->angleDelta().y() < 0)
    dPtr->camera.increment_zoom(-increment);
  else
    dPtr->camera.increment_zoom(increment);


  this->update();
}

void SGMGraphicsWidget::exec_context_menu(const QPoint &pos)
{
  QMenu menu;
  QAction* reset_camera = menu.addAction(tr("Reset Camera"));
  QAction* result = menu.exec(pos);
  if(result == reset_camera)
      {
      reset_view();
      }
}
