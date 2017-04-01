#pragma once

#include <QtWidgets/QMainWindow>

//ui
#include "ui_TMainWindow.h"

class OSGViewerWidget;

class TMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    TMainWindow(QWidget *parent = nullptr);
    virtual ~TMainWindow();

protected slots:
    void handleQuit();
    void handleAbout();
    void handleUndo();
    void handleNewGame();

protected:
    void createOpenGLContext();

private:

    OSGViewerWidget* m_glWidget;

    Ui::TicTacToeClass m_ui;
};
