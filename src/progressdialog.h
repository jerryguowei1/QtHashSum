// QtHashSum: File Checksum Integrity Verifier & Duplicate File Finder
// Copyright (C) 2018-2020  Faraz Fallahi <fffaraz@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

#include "filehasher.h"

struct ProgressData
{
    QProgressBar *pb = nullptr;
    QLabel *label = nullptr;
};

namespace Ui
{
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ProgressDialog(const QVector<FileHasher *> &jobs, QWidget *parent);
    ~ProgressDialog();

  private:
    Ui::ProgressDialog *ui;

    void timer_timeout();
    void updateProgress();
    void allDone();

    QVector<FileHasher *> m_jobs;
    QVector<ProgressData> m_pds;

    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
};
