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
#include <QProcess>
#include <QImage>
#include <QFileInfo>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("一键环境启动器 (高级版)");
    this->resize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // --- (新增) 顶部：全局设置与说明 ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    fileNameInput = new QLineEdit(this);
    fileNameInput->setPlaceholderText("自定义生成的文件名 (默认: 一键启动)");
    fileNameInput->setMaximumWidth(300);

    QPushButton *btnHelp = new QPushButton("❓ 使用说明", this);
    btnHelp->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc; padding: 5px;");

    topLayout->addWidget(new QLabel("项目名称:"));
    topLayout->addWidget(fileNameInput);
    topLayout->addStretch();
    topLayout->addWidget(btnHelp);

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
    // (升级) 开启多选模式，支持按住 Ctrl 或 Shift 批量选择
    pathList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QPushButton *btnVerify = new QPushButton("验证选中路径", this);
    QPushButton *btnRemove = new QPushButton("批量删除选中", this); // 改为批量删除

    QHBoxLayout *listActionLayout = new QHBoxLayout();
    listActionLayout->addWidget(btnVerify);
    listActionLayout->addWidget(btnRemove);
    listActionLayout->addStretch();

    // --- (新增) 3. 图标选择与生成区域 ---
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QPushButton *btnSelectIcon = new QPushButton("更换快捷方式图标(.ico)", this);
    QPushButton *btnGenerate = new QPushButton("生成脚本并创建桌面快捷方式", this);
    btnGenerate->setMinimumHeight(45);
    btnGenerate->setStyleSheet("background-color: #2E8B57; color: white; font-weight: bold; font-size: 14px;");

    bottomLayout->addWidget(btnSelectIcon);
    bottomLayout->addWidget(btnGenerate, 1); // 权重设为1，让生成按钮更长

    // --- 拼装布局 ---
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(new QLabel("待处理的路径列表 (支持 Ctrl/Shift 多选)："));
    mainLayout->addWidget(pathList);
    mainLayout->addLayout(listActionLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(bottomLayout);

    // --- 绑定信号与槽 ---
    connect(btnHelp, &QPushButton::clicked, this, &MainWindow::showHelpDialog);
    connect(btnBrowse, &QPushButton::clicked, this, &MainWindow::browsePath);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addPath);
    connect(btnVerify, &QPushButton::clicked, this, &MainWindow::verifyPath);
    connect(btnRemove, &QPushButton::clicked, this, &MainWindow::removeSelectedPaths);
    connect(btnSelectIcon, &QPushButton::clicked, this, &MainWindow::selectIcon);
    connect(btnGenerate, &QPushButton::clicked, this, &MainWindow::generateBatAndShortcut);
}

MainWindow::~MainWindow() { delete ui; }

// 功能实现：显示帮助说明 (支持富文本和超链接)
void MainWindow::showHelpDialog() {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("使用说明");
    msgBox.setTextFormat(Qt::RichText); // 开启富文本支持
    msgBox.setText(
        "<h3>欢迎使用一键环境启动器</h3>"
        "<p><b>1. 添加路径：</b>输入网页链接或选择本地程序，点击添加。</p>"
        "<p><b>2. 批量删除：</b>在列表中按住 <b>Ctrl</b> 或 <b>Shift</b> 键可多选，然后点击删除。</p>"
        "<p><b>3. 更换图标：</b>点击下方按钮，选择一个 <code>.ico</code> 格式的图片作为桌面快捷方式的封面。</p>"
        "<br>"
        "<p>📺 <a href='https://www.bilibili.com/video/BV1LawazBEDw/?spm_id_from=333.1387.upload.video_card.click'>点击这里观看视频教程</a></p>" // 换成你实际的视频链接
        );
    msgBox.exec();
}

// 功能实现：浏览文件 (支持按住 Ctrl/Shift 批量多选文件)
void MainWindow::browsePath() {
    // 注意：这里使用的是 getOpenFileNames (结尾有s)，它允许用户在对话框中多选
    QStringList paths = QFileDialog::getOpenFileNames(this, "批量选择需要打开的文件或程序", "", "All Files (*)");

    if (paths.isEmpty()) {
        return; // 用户点击了取消
    }

    if (paths.size() == 1) {
        // 如果用户只选了一个文件，保持原有逻辑，填入输入框
        pathInput->setText(paths.first());
    } else {
        // 如果用户选了多个文件，直接批量添加到下方的 List 列表中！
        pathList->addItems(paths);

        // 可选：清空一下输入框，并给个友好的弹窗提示
        pathInput->clear();
        QMessageBox::information(this, "批量添加成功",
                                 QString("太棒了！已为您直接批量添加了 %1 个文件路径到列表中。").arg(paths.size()));
    }
}

// 功能实现：添加路径
void MainWindow::addPath() {
    QString path = pathInput->text().trimmed();
    if (path.isEmpty()) return;
    pathList->addItem(path);
    pathInput->clear();
}

// 功能实现：验证路径
void MainWindow::verifyPath() {
    if (pathList->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选中需要验证的路径！");
        return;
    }
    // 只验证选中的第一个
    QString path = pathList->selectedItems().first()->text();
    if (!QDesktopServices::openUrl(QUrl::fromUserInput(path))) {
        QMessageBox::critical(this, "验证失败", "无法打开：\n" + path);
    }
}

// 功能实现：批量删除选中路径
void MainWindow::removeSelectedPaths() {
    // 获取所有被选中的 item
    QList<QListWidgetItem*> items = pathList->selectedItems();
    if (items.isEmpty()) return;

    // 遍历删除
    for (QListWidgetItem* item : items) {
        delete pathList->takeItem(pathList->row(item));
    }
}


// 功能实现：选择图标 (尊享版：支持自动转换，并提供居中裁剪与直接拉伸两种选项)
void MainWindow::selectIcon() {
    QString path = QFileDialog::getOpenFileName(this, "选择快捷方式图标或图片", "", "Images (*.ico *.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    QFileInfo fileInfo(path);

    // 1. 如果本来就是 ICO，直接用
    if (fileInfo.suffix().toLower() == "ico") {
        customIconPath = path;
        QMessageBox::information(this, "成功", "已选择原生的 ICO 图标！");
        return;
    }

    // 2. 如果是普通图片，读取并准备转换
    QImage image(path);
    if (image.isNull()) {
        QMessageBox::warning(this, "错误", "图片读取失败，请检查文件是否损坏！");
        return;
    }

    QImage finalIcon;

    // --- 核心交互：让用户选择处理方式 ---
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("图片处理选项");
    msgBox.setText("您选择了一张普通图片，生成快捷方式需要 1:1 的正方形图标。\n请选择您喜欢的处理方式：");

    // 添加自定义按钮
    QPushButton *btnCrop = msgBox.addButton("✂️ 居中裁剪 (推荐，不变形)", QMessageBox::ActionRole);
    QPushButton *btnStretch = msgBox.addButton("↔️ 直接拉伸 (保留全图，可能变形)", QMessageBox::ActionRole);
    QPushButton *btnCancel = msgBox.addButton("取消", QMessageBox::RejectRole);

    msgBox.exec(); // 弹出对话框等待用户点击

    // --- 根据用户的选择执行不同的算法 ---
    if (msgBox.clickedButton() == btnCrop) {
        // 【算法 A：智能居中裁剪】
        int minSize = qMin(image.width(), image.height());
        int x = (image.width() - minSize) / 2;
        int y = (image.height() - minSize) / 2;
        QImage squareImage = image.copy(x, y, minSize, minSize);
        finalIcon = squareImage.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    } else if (msgBox.clickedButton() == btnStretch) {
        // 【算法 B：暴力直接拉伸】
        finalIcon = image.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    } else {
        // 用户点击了取消或关闭了窗口
        return;
    }

    // --- 保存并应用最终的图标 ---
    QString timeStamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString convertedIcoPath = QDir::currentPath() + "/icon_" + timeStamp + ".ico";

    if (finalIcon.save(convertedIcoPath, "ICO")) {
        customIconPath = convertedIcoPath;
        QMessageBox::information(this, "转换成功", "图片已处理完毕！\n\n为了防止系统缓存，已生成独立图标文件：\n" + convertedIcoPath);
    } else {
        QMessageBox::warning(this, "转换失败", "无法将图片保存为 ICO 格式。");
    }
}

// 功能实现：生成脚本与快捷方式 (包含 VBS 黑科技)
void MainWindow::generateBatAndShortcut() {
    if (pathList->count() == 0) {
        QMessageBox::warning(this, "提示", "列表是空的，请先添加路径！");
        return;
    }

    // 1. 处理自定义文件名
    QString baseName = fileNameInput->text().trimmed();
    if (baseName.isEmpty()) baseName = "一键启动"; // 默认名称

    QString batFilePath = QDir::currentPath() + "/" + baseName + ".bat";

    // 2. 写入 Bat 文件
    QFile file(batFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "没有权限或无法创建 Bat 脚本文件！");
        return;
    }

    QTextStream out(&file);
    // Bat 脚本必须用 UTF-8，配合 chcp 65001 防止终端乱码
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    out << "chcp 65001\n@echo off\n\n";
    for (int i = 0; i < pathList->count(); ++i) {
        QString path = pathList->item(i)->text();
        path.replace("/", "\\");
        out << "start \"\" \"" << path << "\"\n";
    }
    file.close();

    // 3. 创建桌面快捷方式 (分情况处理)
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString shortcutPath = desktopPath + "/" + baseName + ".lnk";

    // 统一将路径的斜杠转为反斜杠，适配 VBS 和 Windows 系统
    QString winBatPath = batFilePath; winBatPath.replace("/", "\\");
    QString winShortcutPath = shortcutPath; winShortcutPath.replace("/", "\\");

    if (QFile::exists(shortcutPath)) {
        QFile::remove(shortcutPath);
    }

    bool success = false;

    // 如果用户选择了自定义图标，使用 VBS 脚本创建带图标的快捷方式
    if (!customIconPath.isEmpty()) {
        QString winIconPath = customIconPath; winIconPath.replace("/", "\\");

        // 创建临时的 vbs 脚本文件
        QString vbsPath = QDir::currentPath() + "/temp_create_shortcut.vbs";
        QFile vbsFile(vbsPath);
        if (vbsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream vbsOut(&vbsFile);

            // ================= 【核心修复点：解决中文路径导致 VBS 失败】 =================
            // 强制让 VBS 脚本使用 Windows 系统的本地编码 (ANSI/GBK)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            vbsOut.setCodec(QTextCodec::codecForLocale());
#else
            vbsOut.setEncoding(QStringConverter::System);
#endif
            // =========================================================================

            vbsOut << "Set oWS = WScript.CreateObject(\"WScript.Shell\")\n";
            vbsOut << "sLinkFile = \"" << winShortcutPath << "\"\n";
            vbsOut << "Set oLink = oWS.CreateShortcut(sLinkFile)\n";
            vbsOut << "oLink.TargetPath = \"" << winBatPath << "\"\n";
            vbsOut << "oLink.IconLocation = \"" << winIconPath << "\"\n"; // 设置图标
            vbsOut << "oLink.Save\n";
            vbsFile.close();

            // 执行 VBS 脚本
            QProcess::execute("wscript", QStringList() << vbsPath);
            // 删掉临时脚本，做到无痕
            QFile::remove(vbsPath);

            // 检查快捷方式是否真的生成成功了
            success = QFile::exists(shortcutPath);
        }
    } else {
        // 如果没有选图标，使用 Qt 原生方法创建普通快捷方式
        success = QFile::link(batFilePath, shortcutPath);
    }

    // 4. 弹窗提示最终结果
    if (success) {
        QMessageBox::information(this, "大功告成",
                                 QString("项目【%1】已生成！\n\n去桌面看看带图标的快捷方式吧！").arg(baseName));
    } else {
        QMessageBox::warning(this, "部分成功",
                             QString("Bat脚本生成成功，但快捷方式创建失败。\n\n排查建议：\n1. 请检查是否有杀毒软件拦截\n2. 请确认路径是否有特殊字符\n\n脚本位置: %1").arg(batFilePath));
    }
}