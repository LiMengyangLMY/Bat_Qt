#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- 界面基础设置 ---
    this->setWindowTitle("自动生成Bat脚本工具");
    this->resize(550, 400);

    // 创建一个中心部件来承载我们的布局 (覆盖掉ui文件默认的内容)
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // --- 1. 添加所需文件区域 ---
    QHBoxLayout *inputLayout = new QHBoxLayout();
    pathInput = new QLineEdit(this);
    pathInput->setPlaceholderText("请输入网页URL或本地文件路径...");
    QPushButton *btnBrowse = new QPushButton("浏览文件...", this);
    QPushButton *btnAdd = new QPushButton("添加到列表", this);

    inputLayout->addWidget(pathInput);
    inputLayout->addWidget(btnBrowse);
    inputLayout->addWidget(btnAdd);

    // --- 2. 显示文件路径列表及验证区域 ---
    pathList = new QListWidget(this);
    QPushButton *btnVerify = new QPushButton("验证选中路径能否打开", this);
    QPushButton *btnRemove = new QPushButton("删除选中路径", this);

    QHBoxLayout *listActionLayout = new QHBoxLayout();
    listActionLayout->addWidget(btnVerify);
    listActionLayout->addWidget(btnRemove);
    listActionLayout->addStretch(); // 把按钮顶到左边

    // --- 3. 自动生成bat脚本区域 ---
    QPushButton *btnGenerate = new QPushButton("生成Bat脚本并创建桌面快捷方式", this);
    btnGenerate->setMinimumHeight(45); // 让主按钮大一点
    // 给主按钮加点颜色，提升交互体验
    btnGenerate->setStyleSheet("background-color: #2E8B57; color: white; font-weight: bold; font-size: 14px;");

    // --- 将所有部件拼装到主布局 ---
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(new QLabel("待处理的路径列表："));
    mainLayout->addWidget(pathList);
    mainLayout->addLayout(listActionLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(btnGenerate);

    // --- 绑定信号与槽 (点击按钮触发对应的函数) ---
    connect(btnBrowse, &QPushButton::clicked, this, &MainWindow::browsePath);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addPath);
    connect(btnVerify, &QPushButton::clicked, this, &MainWindow::verifyPath);
    connect(btnRemove, &QPushButton::clicked, [this](){
        // 使用 Lambda 表达式快速实现删除功能
        delete pathList->takeItem(pathList->currentRow());
    });
    connect(btnGenerate, &QPushButton::clicked, this, &MainWindow::generateBatAndShortcut);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 功能实现：浏览文件
void MainWindow::browsePath() {
    QString path = QFileDialog::getOpenFileName(this, "选择需要打开的文件或程序");
    if (!path.isEmpty()) {
        pathInput->setText(path);
    }
}

// 功能实现：添加路径到列表
void MainWindow::addPath() {
    QString path = pathInput->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::information(this, "提示", "路径不能为空哦！");
        return;
    }
    pathList->addItem(path);
    pathInput->clear();
}

// 功能实现：验证路径正确性
void MainWindow::verifyPath() {
    QListWidgetItem *item = pathList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先在列表中点击选中一条需要验证的路径！");
        return;
    }

    QString path = item->text();
    // 使用 fromUserInput 可以同时兼容网页(http://)和本地文件(C:/...)
    bool success = QDesktopServices::openUrl(QUrl::fromUserInput(path));
    if (!success) {
        QMessageBox::critical(this, "验证失败", "系统无法打开该路径，请检查路径是否拼写正确或文件是否存在！\n\n路径：" + path);
    }
}

// 功能实现：生成脚本与快捷方式
void MainWindow::generateBatAndShortcut() {
    if (pathList->count() == 0) {
        QMessageBox::warning(this, "提示", "列表是空的，请先添加路径！");
        return;
    }

    // 1. 准备 Bat 文件路径 (默认放在程序运行目录下)
    QString batFilePath = QDir::currentPath() + "/AutoStart.bat";
    QFile file(batFilePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "没有权限或无法创建 Bat 脚本文件！");
        return;
    }

    QTextStream out(&file);

    // 强制使用 UTF-8 编码，这是匹配 chcp 65001 的关键所在
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    // 严格按照 README 的格式写入内容
    out << "chcp 65001\n";
    out << "::可正常显示中文\n";
    out << "@echo off\n\n";

    for (int i = 0; i < pathList->count(); ++i) {
        QString path = pathList->item(i)->text();
        // 关键细节：将正斜杠 / 替换为 Windows 认识的反斜杠 \
        path.replace("/", "\\");
        out << "start \"\" \"" << path << "\"\n";
    }
    file.close();

    // 2. 准备创建桌面快捷方式
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString shortcutPath = desktopPath + "/一键启动环境.lnk";

    // 如果桌面已有同名快捷方式，先删掉防止冲突
    if (QFile::exists(shortcutPath)) {
        QFile::remove(shortcutPath);
    }

    // Qt 原生 API 创建快捷方式
    bool linkSuccess = QFile::link(batFilePath, shortcutPath);

    if (linkSuccess) {
        QMessageBox::information(this, "大功告成",
                                 QString("Bat脚本已成功生成！\n\n脚本位置: %1\n\n已自动在桌面创建名为【一键启动环境】的快捷方式，去桌面看看吧！").arg(batFilePath));
    } else {
        QMessageBox::warning(this, "部分成功",
                             QString("Bat脚本生成成功，但在桌面创建快捷方式失败。\n脚本位置: %1").arg(batFilePath));
    }
}