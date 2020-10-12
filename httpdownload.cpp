#include "httpdownload.h"
#include "ui_httpdownload.h"
#include "iostream"

HttpDownload::HttpDownload(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HttpDownload),
    index(0)
{
    ui->setupUi(this);
    ui->urlEdit->setText("https://raw.githubusercontent.com/webber04/HydraTableMaps/master/");
    ui->statusLabel->setWordWrap(true);
    ui->downloadButton->setDefault(true);
    ui->quitButton->setAutoDefault(false);

    ui->localPath->setText("/home/selman/GitChecker/HydraTableMaps/");

    connect(ui->urlEdit, SIGNAL(textChanged(QString)), this, SLOT(enableDownloadButton()));
    connect(this, SIGNAL(nextFile()), this, SLOT(download()));
}

HttpDownload::~HttpDownload()
{
    delete ui;
}

void HttpDownload::on_downloadButton_clicked()
{
    QDir directory(ui->localPath->text());
    directory.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QDirIterator it(directory, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        list.push_back(it.filePath().remove(ui->localPath->text()));
    }

    download();
}


void HttpDownload::download()
{
    manager = new QNetworkAccessManager(this);

    QString fileToDownload = list.at(index);

    // get url
    url = (ui->urlEdit->text() + fileToDownload);

    QFileInfo fileInfo(url.path());
    QString fileName = ui->localPath->text() + "tmp_" + fileInfo.fileName();

    if (QFile::exists(fileName))
    {
        if (QMessageBox::question(this, tr("HTTP"),
                tr("There already exists a file called %1 in "
                "the current directory. Overwrite?").arg(fileName),
                QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
                == QMessageBox::No)
                return;
        QFile::remove(fileName);
    }

    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly))
    {
        QMessageBox::information(this, tr("HTTP"),
                      tr("Unable to save the file %1: %2.")
                      .arg(fileName).arg(file->errorString()));
        delete file;
        file = 0;
        return;
    }

    // used for progressDialog
    // This will be set true when canceled from progress dialog
    httpRequestAborted = false;

    //progressDialog->setWindowTitle(tr("HTTP"));
   // progressDialog->setLabelText(tr("Downloading %1.").arg(fileName));

    // download button disabled after requesting download
    ui->downloadButton->setEnabled(false);

    startRequest(url);
}

void HttpDownload::httpReadyRead()
{
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}

void HttpDownload::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (httpRequestAborted)
        return;

    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(bytesRead);
}

void HttpDownload::on_quitButton_clicked()
{
    this->close();
}

void HttpDownload::on_urlEdit_returnPressed()
{
    on_downloadButton_clicked();
}

void HttpDownload::enableDownloadButton()
{
    ui->downloadButton->setEnabled(!(ui->urlEdit->text()).isEmpty());
}

// During the download progress, it can be canceled
void HttpDownload::cancelDownload()
{
    ui->statusLabel->setText(tr("Download canceled."));
    httpRequestAborted = true;
    reply->abort();
    ui->downloadButton->setEnabled(true);
}

// When download finished or canceled, this will be called
void HttpDownload::httpDownloadFinished()
{
    // when canceled
    if (httpRequestAborted)
    {
        if (file)
        {
            file->close();
            file->remove();
            delete file;
            file = 0;
        }
        reply->deleteLater();
        return;
    }

    QString downloadedFile = file->fileName();
    // download finished normally
    file->flush();
    file->close();

    // get redirection url
//    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
//    if (reply->error()) {
//        file->remove();
//        QMessageBox::information(this, tr("HTTP"),
//                                 tr("Download failed: %1.")
//                                 .arg(reply->errorString()));
//        ui->downloadButton->setEnabled(true);
//    } else if (!redirectionTarget.isNull()) {
//        QUrl newUrl = url.resolved(redirectionTarget.toUrl());
//        if (QMessageBox::question(this, tr("HTTP"),
//                                  tr("Redirect to %1 ?").arg(newUrl.toString()),
//                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
//            url = newUrl;
//            reply->deleteLater();
//            file->open(QIODevice::WriteOnly);
//            file->resize(0);
//            startRequest(url);
//            return;
//        }
//    } else {
//        QString fileName = QFileInfo(QUrl(ui->urlEdit->text()).path()).fileName();
//        ui->statusLabel->setText(tr("Downloaded %1 to %2.").arg(fileName).arg(QDir::currentPath()));
//        ui->downloadButton->setEnabled(true);
//    }


    reply->deleteLater();
    reply = 0;

    manager = 0;

    QString localFile = ui->localPath->text() + list.at(index);

    //std::cout << "Local:" << localFile.toStdString() << std::endl;
    //std::cout << "Remote:" << downloadedFile.toStdString() << std::endl;

    if (fileChecksum(localFile, QCryptographicHash::Sha1) == fileChecksum(downloadedFile, QCryptographicHash::Sha1))
    {
        //std::cout << "" << localFile.toStdString() << " is the same" << std::endl;
    }
    else
    {
        std::cout << "" << localFile.toStdString() << " is modifed" << std::endl;
        QFile(localFile).remove();
        file->copy(localFile);
    }

    file->remove();

    delete file;
    file = 0;

    index++;

    if (index < list.size())
    {
        emit nextFile();
        updateDownloadProgress(index, list.size());
    }
    else
    {
        index = 0;
        updateDownloadProgress(list.size(), list.size());
        ui->downloadButton->setEnabled(true);
    }

}

// This will be called when download button is clicked
void HttpDownload::startRequest(QUrl url)
{
    // get() method posts a request
    // to obtain the contents of the target request
    // and returns a new QNetworkReply object
    // opened for reading which emits
    // the readyRead() signal whenever new data arrives.
    reply = manager->get(QNetworkRequest(url));

    // Whenever more data is received from the network,
    // this readyRead() signal is emitted
    connect(reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));

    // This signal is emitted when the reply has finished processing.
    // After this signal is emitted,
    // there will be no more updates to the reply's data or metadata.
    connect(reply, SIGNAL(finished()), this, SLOT(httpDownloadFinished()));
}

QByteArray HttpDownload::fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile file(fileName);
    QCryptographicHash hash(QCryptographicHash::Sha1);

    if( file.open(QFile::ReadOnly ) )
    {
        QByteArray header = QString("blob %1").arg(file.size()).toUtf8();
        hash.addData(header.data(), header.size() + 1);
        hash.addData(file.readAll());
    }
    else
    {
        std::cout << "Error opening " << fileName.toStdString() << std::endl;
    }

    file.close();
    return hash.result().toHex();
}
