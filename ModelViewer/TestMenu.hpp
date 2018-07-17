#ifndef TESTMENU_HPP
#define TESTMENU_HPP

#include <QMenu>

class TestMenu : public QMenu
{
Q_OBJECT
public:
    explicit TestMenu(QWidget *parent = Q_NULLPTR);

    ~TestMenu() override = default;

signals:

    void all();

    void number();

    void script();

    void check();
};

#endif // TESTMENU_HPP
