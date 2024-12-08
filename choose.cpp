      
//该文件是选择备份还是恢复界面
//需要陈美化选择备份与恢复界面

#include "choose.h"
#include "filepacker.h"
#include "up_down.h"
#include "ui_choose.h"

#include <QGraphicsDropShadowEffect>

using namespace std;

choose::choose(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::choose)
{
    ui->setupUi(this);

    //设置图片
    QPixmap *pix = new QPixmap(":/flower.png");
    QSize sz = ui->label_image->size();
    ui->label_image->setPixmap(pix->scaled(sz));
    //设置图片阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(3, 0);
    shadow->setColor(QColor("#888888"));
    shadow->setBlurRadius(30);
    ui->label_image->setGraphicsEffect(shadow);

    //connect(ui->restore,SIGNAL(clicked()),this,SLOT(on_restore_clicked()));
    //connect(ui->backup,SIGNAL(clicked()),this,SLOT(on_backup_clicked()));

}

choose::~choose()
{
    delete ui;
}

void choose::on_backup_clicked() //点击备份按钮
{
    QString fileName;

    //这一个QMessageBox的功能为选择备份目录还是文件
    QMessageBox *messagebox  = new QMessageBox(this);
    messagebox->setBaseSize(600,300);
    messagebox->setWindowTitle("选择...");
    messagebox->setText("请选择备份文件或目录");
    QPushButton *file =messagebox->addButton("文件",QMessageBox::RejectRole);
    QPushButton *dir = messagebox->addButton("目录",QMessageBox::AcceptRole);
    messagebox->exec();
    if(messagebox->clickedButton() == file)
        fileName = QFileDialog::getOpenFileName(this,"选择文件","/home/czy");
    else if(messagebox->clickedButton() == dir)
        fileName = QFileDialog::getExistingDirectory(this,"选择目录","/home/czy");

    char t[] = "/tmp/BackupFile.XXXXXX";
    QString tempDirectory = mkdtemp(t);   //创建临时文件
    tempDirectory +="/";
    qDebug() << "这是一条调试信息" << tempDirectory;
    //get the backup_file
    if (!fileName.isEmpty()) {
       qDebug() << "this is the file name " << fileName;
    }
    if(fileName.back() == '/')
        fileName.left(fileName.size() - 1);
    int pos = fileName.lastIndexOf('/',fileName.size() - 2);

    QString relativePath = fileName.right(fileName.size() - pos - 1);
    QString rootDirectory = fileName.left(pos+1);
    qDebug() << " test0 " << pos;
    qDebug() << " test1 " << relativePath;
    qDebug() << " test2 " << rootDirectory;
    //文件打包
    QMessageBox::information(NULL, "", "正在打包中...");
    FilePacker Packer = FilePacker(tempDirectory.toStdString(),true); 
    delete InodeRecorder::inodeRecorderBackup;
    InodeRecorder::inodeRecorderBackup = new InodeRecorderBackup();
    FileInfo *fileInfo = new FileInfo(relativePath.toStdString(),&Packer,rootDirectory.toStdString());
    if (fileInfo->Backup() != NO_ERROR)
    {
        delete fileInfo;
        fileInfo = nullptr;
        printf("打包失败！\n");
        return;
    }
    delete fileInfo;
    fileInfo = nullptr;
    Packer.Compact();
    QMessageBox::information(NULL, "", "文件打包完成");

    //文件压缩
    QMessageBox::information(NULL, "", "正在压缩中...");
    FileCompressor *fileCompressor = new FileCompressor(tempDirectory.toStdString(),Packer.BackupFile);    
    if(fileCompressor->Compress() != NO_ERROR)
    {
        delete fileCompressor;
        printf("压缩失败！\n");
        return;
    }
    QMessageBox::information(NULL, "", "文件压缩完成");

    //文件上传
    QMessageBox::information(NULL, "", "文件上传中...");
    if(upload((tempDirectory + COMPRESSOR_FILE_NAME).toStdString().c_str(),name.toStdString().c_str()) == 1)
    {
        qDebug() << "test4 ";
        fileCompressor->DeleteFile();
        delete fileCompressor;
        Packer.DeleteFile();
        printf("ALL PACKUP PROCESS FINISHED SUCCESSFULLY!\n");
        system((string("rm -R ") + tempDirectory.toStdString()).c_str());
    }
    else
    {
        delete fileCompressor;
        printf("Backup failed! Because unknown error occurred on uploading.\n");
        printf("Backup File store in %s",tempDirectory);
    }
}

void choose::on_restore_clicked()
{
    char tr[] = "/tmp/BackupToolTmpFile.XXXXXX";
    QString tempDirectory = mkdtemp(tr);
    tempDirectory += "/";

    //选择目录界面，并将所得目录传递给变量rootDirectory
    QString rootDirectory = QFileDialog::getExistingDirectory(this,"请选择目标文件夹","/home/czy");
    if (!rootDirectory.isEmpty()) {
       qDebug() << "this is the file name " << rootDirectory;
    }
    if (rootDirectory.back() != '/')
            rootDirectory += '/';

    QString packDirecroty_user = "/home/czy/packup/" + name;
    QString packDirectory = QFileDialog::getOpenFileName(this,"请选择文件",packDirecroty_user);
    if (!packDirectory.isEmpty()) {
       qDebug() << "this is the file name " << packDirectory;
    }

    QMessageBox::information(NULL, "", "正在下载中...");
    if(download(packDirectory.toStdString().c_str(),tempDirectory.toStdString().c_str())==1){ //下载备份的文件
        QMessageBox::information(NULL, "", "文件下载成功！");
        //解压缩
        QMessageBox::information(NULL, "", "正在解压中...");
        FileCompressor *fileCompressor = new FileCompressor(tempDirectory.toStdString());
        if(fileCompressor->Decompress() != NO_ERROR)
        {
            delete fileCompressor;
            printf("解压失败！\n");
            return;
            //error
        }
        fileCompressor->DeleteFile();
        delete fileCompressor;
        QMessageBox::information(NULL, "", "文件解压成功！");
        //解包
        QMessageBox::information(NULL, "", "正在解包中...");
        FilePacker Packer = FilePacker(tempDirectory.toStdString(),false);
        Packer.Disassemble();
        delete InodeRecorder::inodeRecorderRestore;
        InodeRecorder::inodeRecorderRestore = new InodeRecorderRestore();
        off_t ProcessBarTotal = Packer.UnitFile->Length();
        off_t ProcessBarCurrent{0};
        while(Packer.UnitFile->peek() != EOF)
        {
            FileInfo *fileInfo = new FileInfo(&Packer,rootDirectory.toStdString());
            if(fileInfo->Restore() != NO_ERROR)
            {
                delete fileInfo;
                fileInfo = nullptr;
                QMessageBox::information(NULL, "", "文件恢复失败！");
                printf("恢复失败！\n");
                return;
                //error
            }
            char adjust = (ProcessBarCurrent>>20) & 0x1;
            ProcessBarCurrent = Packer.UnitFile->tellg();
            if(((ProcessBarCurrent>>20) & 0x1) == adjust + 1)
                cout << "\rRestoring: (" << ProcessBarCurrent << " / " << ProcessBarTotal << ")" << endl;
            delete fileInfo;
            fileInfo = nullptr;
        }
        QMessageBox::information(NULL, "", "文件解包完成！");
        Packer.DeleteFile();
        cout << "Restoring: (" << ProcessBarTotal << " / " << ProcessBarTotal << ")" << endl;
        QMessageBox::information(NULL, "", "文件恢复成功！");
        cout << "All restore process finished successfully!" << endl;
        system((string("rm -R ") + tempDirectory.toStdString()).c_str());
    }
    else
    {
        printf("download fail!\n");
        QMessageBox::warning(NULL, "", "文件下载失败！请重试。");
        system((string("rm -R ") + tempDirectory.toStdString()).c_str());
    }
        
}

void choose::getData(QString str1)
{
    //获取从mainwindow界面登录成功的用户名
    name = str1;
}


    
