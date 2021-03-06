#include "MainWindow.hpp"
#include "moc_MainWindow.cpp"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "qinputdialog.h"
#include "qmessagebox.h"
#include "qsizepolicy.h"

#include "FileMenu.hpp"
#include "ViewMenu.hpp"
#include "TestMenu.hpp"
#include "PrimitiveMenu.hpp"
#include "ModelData.hpp"

#include "SGMChecker.h"
#include "SGMPrimitives.h"
#include "SGMVector.h"
#include "SGMEntityClasses.h"
#include "SGMDisplay.h"

#include <map>
#include <cstdio>

#ifdef VIEWER_WITH_GTEST
#include "TestDialog.hpp"
#endif

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  mModel(new ModelData),
  ui(new Ui::MainWindow),
  mFileMenu(new FileMenu),
  mViewMenu(new ViewMenu),
  mTestMenu(new TestMenu),
  mPrimitiveMenu(new PrimitiveMenu),
  mTestDialog(nullptr)
{
  ui->setupUi(this);
  ui->mGraphics->setFocusPolicy(Qt::ClickFocus);

  ui->menubar->addMenu(mFileMenu);
  connect(mFileMenu, SIGNAL(open()),
          this, SLOT(file_open()));
  connect(mFileMenu, SIGNAL(recent_file(QString)),
          this, SLOT(file_open_recent(QString)));
  connect(mFileMenu, SIGNAL(sgm()),
          this, SLOT(file_sgm()));
  connect(mFileMenu, SIGNAL(step()),
          this, SLOT(file_step()));
  connect(mFileMenu, SIGNAL(stl()),
          this, SLOT(file_stl()));
  connect(mFileMenu, SIGNAL(exit()),
          this, SLOT(file_exit()));

  ui->menubar->addMenu(mViewMenu);
  connect(mViewMenu, SIGNAL(faces()),
          this, SLOT(view_faces()));
  connect(mViewMenu, SIGNAL(edges()),
          this, SLOT(view_edges()));
  connect(mViewMenu, SIGNAL(vertices()),
          this, SLOT(view_vertices()));
  connect(mViewMenu, SIGNAL(facet()),
          this, SLOT(view_facet()));
  connect(mViewMenu, SIGNAL(uvspace()),
          this, SLOT(view_uvspace()));
  connect(mViewMenu, SIGNAL(perspective()),
          this, SLOT(view_perspective()));
  connect(mViewMenu, SIGNAL(background()),
          this, SLOT(view_background()));

  ui->menubar->addMenu(mTestMenu);
  connect(mTestMenu, SIGNAL(check()),
          this, SLOT(test_check()));
  connect(mTestMenu, SIGNAL(free_edges()),
          this, SLOT(test_free_edges()));
#ifdef VIEWER_WITH_GTEST
  connect(mTestMenu, SIGNAL(gtest()),
          this, SLOT(test_gtest()));
#endif

  ui->menubar->addMenu(mPrimitiveMenu);
  connect(mPrimitiveMenu, SIGNAL(block()),
          this, SLOT(primitive_block()));
  connect(mPrimitiveMenu, SIGNAL(sphere()),
          this, SLOT(primitive_sphere()));
  connect(mPrimitiveMenu, SIGNAL(cylinder()),
          this, SLOT(primitive_cylinder()));
  connect(mPrimitiveMenu, SIGNAL(cone()),
          this, SLOT(primitive_cone()));
  connect(mPrimitiveMenu, SIGNAL(torus()),
          this, SLOT(primitive_torus()));
  connect(mPrimitiveMenu, SIGNAL(line()),
          this, SLOT(primitive_line()));
  connect(mPrimitiveMenu, SIGNAL(circle()),
          this, SLOT(primitive_circle()));
  connect(mPrimitiveMenu, SIGNAL(ellipse()),
          this, SLOT(primitive_ellipse()));
  connect(mPrimitiveMenu, SIGNAL(parabola()),
          this, SLOT(primitive_parabola()));
  connect(mPrimitiveMenu, SIGNAL(hyperbola()),
          this, SLOT(primitive_hyperbola()));
  connect(mPrimitiveMenu, SIGNAL(NUBcurve()),
          this, SLOT(primitive_NUBcurve()));
  connect(mPrimitiveMenu, SIGNAL(TorusKnot()),
          this, SLOT(primitive_torus_knot()));
  connect(mPrimitiveMenu, SIGNAL(Complex()),
          this, SLOT(primitive_complex()));
  connect(mPrimitiveMenu, SIGNAL(revolve()),
          this, SLOT(primitive_revolve()));

  mModel->set_tree_widget(ui->twTree);
  mModel->set_graphics_widget(ui->mGraphics);
  
  read_settings();
}

MainWindow::~MainWindow()
{
  delete mFileMenu;
  delete mViewMenu;
  delete mTestMenu;
  delete mPrimitiveMenu;
  delete ui;

  delete mModel;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  save_settings();
  QMainWindow::closeEvent(event);
}

void MainWindow::file_open()
{
  QString type_filter = tr("SGM files (%1)").arg("*.sgm");
  type_filter += ";;" + tr("STL Files (%1)").arg("*.stl");
  type_filter += ";;" + tr("Step Files (%1)").arg("*.stp *.step");
  type_filter += ";;" + tr("Text Files (%1)").arg("*.txt");
  type_filter += ";;" + tr("All files").arg("*.*");

  // Run the dialog
  QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Open File(s)"), "", type_filter);

  for(const QString &f : files)
    file_open_recent(f);
}

void MainWindow::file_sgm()
{
  QString type_filter = tr("SGM files (%1)").arg("*.sgm");
  QString SaveName=QFileDialog::getSaveFileName(this, tr("Output File"), "", type_filter);

  mModel->sgm(SaveName);
}

void MainWindow::file_step()
{
  QString type_filter = tr("Step files (%1)").arg("*.stp *.step");
  QString SaveName=QFileDialog::getSaveFileName(this, tr("Output File"), "", type_filter);

  mModel->step(SaveName);
}

void MainWindow::file_stl()
{
  QString type_filter = tr("STL Files (%1)").arg("*.stl");
  QString SaveName=QFileDialog::getSaveFileName(this, tr("Output File"), "", type_filter);

  mModel->stl(SaveName);
}

void MainWindow::file_exit()
{
  this->close();
}

#define SGM_ADD_ALL_FILES_TO_LIST 1

void MainWindow::file_open_recent(const QString &filename)
{
#if SGM_ADD_ALL_FILES_TO_LIST
  mFileMenu->add_recent_file(filename);
#endif
  bool opened = mModel->open_file(filename);
  if(opened)
    mFileMenu->add_recent_file(filename);
}

void MainWindow::read_settings()
{
  QSettings settings;
  if(settings.contains("MainWindow/geometry"))
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
  else
  {
    // Center the main window
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen_geom = desktop->screenGeometry();
    QRect main_geom = geometry();
    move(screen_geom.center() - main_geom.center());
  }

  if(settings.contains("MainWindow/state"))
    restoreState(settings.value("MainWindow/state").toByteArray());
}

/*
void MainWindow::view_zoom()
    {
    QDialog ViewOptions;
    ViewOptions.setWindowTitle("View Options");
    ViewOptions.exec();
    }
*/

void MainWindow::view_faces()
    {
    mModel->face_mode();
    }

void MainWindow::view_edges()
    {
    mModel->edge_mode();
    }

void MainWindow::view_vertices()
    {
    mModel->vertex_mode();
    }

void MainWindow::view_facet()
    {
    mModel->facet_mode();
    }

void MainWindow::view_uvspace()
    {
    mModel->uvspace_mode();
    }

void MainWindow::view_perspective()
    {
    mModel->perspective_mode();
    }

void MainWindow::view_background()
    {
    mModel->set_background();
    }

void MainWindow::test_check()
{
  std::vector<std::string> aLog;
  mModel->check(aLog);

  if(aLog.empty())
      {
      QMessageBox Msgbox;
        Msgbox.setText("All Entities Checked               ");
        Msgbox.exec();
      }
  else
      {
      std::string Message;
      size_t nLog=aLog.size();
      size_t Index1;
      for(Index1=0;Index1<nLog;++Index1)
          {
          Message+=aLog[Index1];
          if(1<nLog && Index1<nLog-1)
              {
              Message+="\n";
              }
          }
      QMessageBox Msgbox;
        Msgbox.setText(Message.c_str());
        Msgbox.exec();
        Msgbox.setSizeGripEnabled(true);
      }
}


void MainWindow::test_free_edges()
{
  mModel->free_edges();
}

#ifdef VIEWER_WITH_GTEST
void gtest_result_dialog_exec(int ret_value, const char* arg)
{
    QFile out_file("~gtest_stdout.log");
    out_file.open(QIODevice::ReadOnly);
    QByteArray output = out_file.readAll();
    QString out_string(output);
    out_file.close();

    QFile err_file("~gtest_stderr.log");
    err_file.open(QIODevice::ReadOnly);
    QByteArray errors = err_file.readAll();
    QString err_string(errors);
    err_file.close();

    QMessageBox msg_box;
    QString title("gtest ");
    QString info;
    msg_box.setTextInteractionFlags(Qt::TextSelectableByMouse);
    msg_box.setWindowTitle(title);
    msg_box.setInformativeText(out_string);
    if (0 == ret_value)
        {
        msg_box.setIcon(QMessageBox::Information);
        info = "PASSED\n";
        }
    else{
        msg_box.setIcon(QMessageBox::Warning);
        info = "FAILED\n";
        msg_box.setDetailedText(err_string);
        }
    info += QString(arg);
    msg_box.setText(info);
    QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msg_box.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msg_box.exec();
}

void MainWindow::test_gtest()
{
    if (mTestDialog == nullptr)
        mTestDialog = new TestDialog(mModel);

    mTestDialog->show();
}
#endif

void MainWindow::primitive_block()
    {
    SGM::Point3D Pos0(0,0,0);
    SGM::Point3D Pos1(1,4,9);
    mModel->create_block(Pos0,Pos1);
    }

void MainWindow::primitive_sphere()
    {
    SGM::Point3D Pos0(0,0,0);
    mModel->create_sphere(Pos0,2.0);
    }

void MainWindow::primitive_cylinder()
    {
    SGM::Point3D Pos0(0,0,0);
    SGM::Point3D Pos1(0,0,1);
    double dRadius=0.5;
    mModel->create_cylinder(Pos0,Pos1,dRadius);
    }

void MainWindow::primitive_cone()
    {
    SGM::Point3D Bottom(0,0,0);
    SGM::Point3D Top(0,0,1);
    double dBottomRadius=1.0;
    double dTopRadius=0.5;
    mModel->create_cone(Bottom,Top,dBottomRadius,dTopRadius);
    }

void MainWindow::primitive_torus()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D Axis(0,0,1);
    mModel->create_torus(Center,Axis,1,3);
    }

void MainWindow::primitive_line()
    {
    SGM::Point3D Origin(0,0,0);
    SGM::UnitVector3D Axis(1,0,0);
    SGM::Interval1D Domain(-2.0,2.0);
    mModel->create_line(Origin,Axis,Domain);
    }

void MainWindow::primitive_circle()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D Normal(0,0,1);
    double dRadius=2.0;
    mModel->create_circle(Center,Normal,dRadius);
    }

void MainWindow::primitive_ellipse()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D XAxis(1,0,0),YAxis(0,1,0);
    double dXRadius=2.0,dYRadius=1.0;
    mModel->create_ellipse(Center,XAxis,YAxis,dXRadius,dYRadius);
    }

void MainWindow::primitive_parabola()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D XAxis(1,0,0),YAxis(0,-1,0);
    double dA=0.5;
    SGM::Interval1D Domain(-2.0,2.0);
    mModel->create_parabola(Center,XAxis,YAxis,dA,Domain);
    }

void MainWindow::primitive_hyperbola()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D XAxis(0,1,0),YAxis(1,0,0);
    double dA=1,dB=1;
    SGM::Interval1D Domain(-2.0,2.0);
    mModel->create_hyperbola(Center,XAxis,YAxis,dA,dB,Domain);
    }

void MainWindow::primitive_NUBcurve()
    {
    std::vector<SGM::Point3D> aPoints;
    aPoints.emplace_back(-2,0,0);
    aPoints.emplace_back(-1,0.5,0);
    aPoints.emplace_back(0,-0.5,0);
    aPoints.emplace_back(1,0.5,0);
    aPoints.emplace_back(2,0,0);
    mModel->create_NUBcurve(aPoints);
    }

void MainWindow::primitive_complex()
    {
    std::vector<SGM::Point3D> aPoints;
    std::vector<unsigned int> aSegments,aTriangles;

    aPoints.push_back(SGM::Point3D(0,0,0));
    aPoints.push_back(SGM::Point3D(1,0,0));
    aPoints.push_back(SGM::Point3D(0,1,0));
    aPoints.push_back(SGM::Point3D(0,0,0.5));
    aPoints.push_back(SGM::Point3D(1,0,0.5));
    aPoints.push_back(SGM::Point3D(0,1,0.5));

    aPoints.push_back(SGM::Point3D(0.5,0,0.5));
    aPoints.push_back(SGM::Point3D(0.5,0.5,0.5));
    aPoints.push_back(SGM::Point3D(0,0.5,0.5));

    aTriangles.push_back(0);
    aTriangles.push_back(1);
    aTriangles.push_back(2);

    aSegments.push_back(0);
    aSegments.push_back(3);
    aSegments.push_back(1);
    aSegments.push_back(4);
    aSegments.push_back(2);
    aSegments.push_back(5);
    
    mModel->create_complex(aPoints,aSegments,aTriangles);
    }

void MainWindow::primitive_torus_knot()
    {
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D XAxis(0,1,0),YAxis(1,0,0);
    size_t nA=2,nB=3;
    double dR=5.0,dr=2;
    mModel->create_torus_knot(Center,XAxis,YAxis,dr,dR,nA,nB);
    }

void MainWindow::primitive_revolve()
    {
    std::vector<SGM::Point3D> aPoints;

    aPoints.push_back(SGM::Point3D(-2,.5,0));
    aPoints.push_back(SGM::Point3D(-1,1.5,0));
    aPoints.push_back(SGM::Point3D(0,1,0));
    aPoints.push_back(SGM::Point3D(1,1.5,0));
    aPoints.push_back(SGM::Point3D(2,2,0));
    SGM::Curve IDCurve = mModel->create_NUBcurve(aPoints);
    SGM::Point3D Origin(-1,0,0);
    SGM::UnitVector3D Axis(1,0,0);
    mModel->create_revolve(Origin, Axis, IDCurve);
   
    }

void MainWindow::save_settings()
{
  QSettings settings;
  settings.setValue("MainWindow/geometry", saveGeometry());
  settings.setValue("MainWindow/state", saveState());
}
