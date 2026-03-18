#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QListWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void browsePath();
    void addPath();
    void verifyPath();
    void removeSelectedPaths();     // (升级) 1. 批量删除路径
    void showHelpDialog();          // (新增) 3. 显示使用说明
    void selectIcon();              // (新增) 4. 选择快捷方式图标
    void generateBatAndShortcut();

private:
    Ui::MainWindow *ui;

    // 界面控件指针
    QLineEdit *fileNameInput;       // (新增) 2. 自定义文件名
    QLineEdit *pathInput;
    QListWidget *pathList;

    // 数据存储
    QString customIconPath;         // (新增) 保存用户选择的ico图标路径
};

#endif // MAINWINDOW_H