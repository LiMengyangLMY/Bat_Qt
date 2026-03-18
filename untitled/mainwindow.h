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
    // 对应 README 中的各项功能
    void browsePath();              // 浏览本地文件
    void addPath();                 // 1. 添加文件路径到列表
    void verifyPath();              // 1. 验证路径正确性
    void generateBatAndShortcut();  // 3. 自动生成bat脚本及桌面快捷方式

private:
    Ui::MainWindow *ui;

    // 我们通过纯代码创建的界面控件
    QLineEdit *pathInput;
    QListWidget *pathList;
};

#endif // MAINWINDOW_H