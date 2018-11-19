#include "progressdialog.h"
#include "ui_progressdialog.h"

#include <QGridLayout>
#include <QDateTime>
#include <QThreadPool>
#include <QDebug>

ProgressDialog::ProgressDialog(QVector<FileHasher *> jobs, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog),
    jobs(jobs)
{
    ui->setupUi(this);
    setWindowTitle("Progress Status (" + QString::number(jobs.size()) + " files)");
    QGridLayout *layout = new QGridLayout(this);
    for(int i = 0; i < qMin(QThreadPool::globalInstance()->maxThreadCount(), jobs.size()); ++i)
    {
        ProgressData pd;
        pd.pb = new QProgressBar(this);
        pd.pb->setMinimum(0);
        pd.pb->setMaximum(100);
        pd.pb->setValue(jobs[i]->percent());
        pd.label = new QLabel(this);
        pd.label->setText(QString::number(i + 1) + " " + jobs[i]->info());
        layout->addWidget(pd.label);
        layout->addWidget(pd.pb);
        pds.append(pd);
    }
    setLayout(layout);
    QCoreApplication::processEvents();
    elapsedtimer.start();
    for(int i = 0; i < jobs.size(); ++i) QThreadPool::globalInstance()->start(jobs[i]);
    qDebug() << "All jobs started" << elapsedtimer.elapsed();
    connect(&timer, &QTimer::timeout, this, &ProgressDialog::timer_timeout);
    timer.start(500);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::timer_timeout()
{
    int used = 0;
    for(int i = 0; i < jobs.size() && used < pds.size(); ++i) if(jobs[i]->started && (!jobs[i]->done || jobs.size() - i <= pds.size() - used))
    {
        pds[used].pb->setValue(jobs[i]->percent());
        pds[used].label->setText(QString::number(i + 1) + "  " + jobs[i]->info());
        used++;
        QCoreApplication::processEvents();
    }
    int done = 0;
    for(int i = 0; i < jobs.size(); ++i) if(jobs[i]->done) done++;
    setWindowTitle("Progress Status (" + QString::number(done) + " / " + QString::number(jobs.size()) + " files done, " + QString::number(100 * done / jobs.size()) + "%)");
    if(done == jobs.size())
    {
        qDebug() << "All jobs done" << elapsedtimer.elapsed();
        timer.stop();

        QString result;
        result.append("Checksums generated by QtHashSum v1.2.0\n");
        result.append("https://github.com/fffaraz/QtHashSum\n");
        result.append(QDateTime::currentDateTime().toString() + "\n");

        QString files;
        qint64 totalsize = 0;
        QMap<QString, int> hashmap; // hash -> count
        QHash<QString, qint64> hashsize; // hash -> size
        QMultiHash<QString, QString> hashpath; // hash -> file path
        for(int i = 0; i < jobs.size(); ++i)
        {
            qint64 size = jobs[i]->size;
            QString hash = jobs[i]->hash;

            totalsize += size;

            files.append(jobs[i]->methodStr() + " " + hash + " " + QString::number(size) + " " + jobs[i]->name() + "\n");

            hashmap[hash] = hashmap[hash] + 1;
            hashpath.insertMulti(hash, jobs[i]->name());

            if(!hashsize.contains(hash)) hashsize.insert(hash, size);
            else if(hashsize.value(hash) != size) qDebug() << "ERROR: same hash different size" << hash;

            delete jobs[i];
        }
        result.append(QString::number(jobs.size()) + " files hashed, " + QString::number(totalsize / 1048576) + " MB total\n");

        QString duplicates;
        int num_duplicates = 0;
        qint64 wasted = 0;
        for(int flag = 0; flag < 2; ++flag)
        {
            for(QMap<QString, int>::const_iterator itr = hashmap.constBegin(); itr != hashmap.constEnd(); ++itr)
            {
                qint64 size = hashsize[itr.key()] / 1048576;
                if(itr.value() > 1 && ((size > 0 && flag == 0) || (size < 1 && flag == 1)))
                {
                    num_duplicates++;
                    wasted += (itr.value() - 1) * hashsize[itr.key()];
                    duplicates.append(QString::number(itr.value()) + " " + QString::number(size) + " " + itr.key() + "\n");
                    QList<QString> list = hashpath.values(itr.key());
                    foreach(QString name, list) duplicates.append("\t" + name + "\n");
                }
            }
        }

        if(num_duplicates > 0)
        {
            result.append(QString::number(num_duplicates) + " duplicates found, " + QString::number(wasted / 1048576) + " MB wasted\n");
            duplicates.append("\n");
        }

        qDebug() << "Result ready" << elapsedtimer.elapsed();

        ResultDialog *rd = new ResultDialog(result + "\n" + duplicates + files);
        rd->show();

        this->deleteLater();
    }
}
