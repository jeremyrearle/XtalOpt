/**********************************************************************
  AbstractEditTab - Generic tab for editing templates

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/ui/abstractedittab.h>

#include <globalsearch/macros.h>
#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/queueinterface.h>

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>

#include <QDebug>

namespace GlobalSearch {

  AbstractEditTab::AbstractEditTab( AbstractDialog *parent, OptBase *p ) :
    AbstractTab(parent, p),
    ui_combo_queueInterfaces(0),
    ui_combo_optimizers(0),
    ui_combo_templates(0),
    ui_edit_user1(0), ui_edit_user2(0), ui_edit_user3(0), ui_edit_user4(0),
    ui_list_edit(0),
    ui_list_optStep(0),
    ui_push_add(0),
    ui_push_help(0),
    ui_push_loadScheme(0),
    ui_push_optimizerConfig(0),
    ui_push_queueInterfaceConfig(0),
    ui_push_remove(0),
    ui_push_saveScheme(0),
    ui_edit_edit(0)
  {
  }

  void AbstractEditTab::initialize()
  {
    ui_edit_edit->setCurrentFont(QFont("Courier"));

    // opt connections
    connect(this, SIGNAL(optimizerChanged(const std::string&)),
            m_opt, SLOT(setOptimizer(const std::string&)),
            Qt::DirectConnection);
    connect(this, SIGNAL(queueInterfaceChanged(const std::string&)),
            m_opt, SLOT(setQueueInterface(const std::string&)),
            Qt::DirectConnection);

    // Dialog connections
    connect(this, SIGNAL(optimizerChanged(const std::string&)),
            m_dialog, SIGNAL(tabsUpdateGUI()));
    connect(this, SIGNAL(queueInterfaceChanged(const std::string&)),
            m_dialog, SIGNAL(tabsUpdateGUI()));

    // Edit tab connections
    connect(this, SIGNAL(optimizerChanged(const std::string&)),
            this, SLOT(populateTemplates()));
    connect(this, SIGNAL(queueInterfaceChanged(const std::string&)),
            this, SLOT(populateTemplates()));
    connect(this, SIGNAL(optimizerChanged(const std::string&)),
            this, SLOT(populateOptStepList()));
    connect(this, SIGNAL(queueInterfaceChanged(const std::string&)),
            this, SLOT(populateOptStepList()));
    connect(ui_push_optimizerConfig, SIGNAL(clicked()),
            this, SLOT(configureOptimizer()));
    connect(ui_push_queueInterfaceConfig, SIGNAL(clicked()),
            this, SLOT(configureQueueInterface()));
    connect(ui_push_help, SIGNAL(clicked()),
            this, SLOT(showHelp()));
    connect(ui_edit_edit, SIGNAL(textChanged()),
            this, SLOT(saveCurrentTemplate()));
    connect(ui_combo_templates, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateEditWidget()));
    connect(ui_push_add, SIGNAL(clicked()),
            this, SLOT(appendOptStep()));
    connect(ui_push_remove, SIGNAL(clicked()),
            this, SLOT(removeCurrentOptStep()));
    connect(ui_list_optStep, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateEditWidget()));
    connect(ui_edit_user1, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui_edit_user2, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui_edit_user3, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui_edit_user4, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui_combo_optimizers, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateOptimizer()));
    connect(ui_combo_queueInterfaces, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateQueueInterface()));
    connect(ui_push_saveScheme, SIGNAL(clicked()),
            this, SLOT(saveScheme()));
    connect(ui_push_loadScheme, SIGNAL(clicked()),
            this, SLOT(loadScheme()));

    // Populate combo boxes
    unsigned int index;

    //  QueueInterfaces
    ui_combo_queueInterfaces->blockSignals(true);
    ui_combo_queueInterfaces->clear();
    index = 0;
    for (QList<QueueInterface*>::const_iterator
           it = m_queueInterfaces.constBegin(),
           it_end = m_queueInterfaces.constEnd();
         it != it_end;
         ++it) {
      ui_combo_queueInterfaces->insertItem(index++,
                                           (*it)->getIDString());
    }
    ui_combo_queueInterfaces->blockSignals(false);

    //  Optimizers
    ui_combo_optimizers->blockSignals(true);
    ui_combo_optimizers->clear();
    index = 0;
    for (QList<Optimizer*>::const_iterator
           it = m_optimizers.constBegin(),
           it_end = m_optimizers.constEnd();
         it != it_end;
         ++it) {
      ui_combo_optimizers->insertItem(index++,
                                      (*it)->getIDString());
    }
    ui_combo_optimizers->blockSignals(false);

    AbstractTab::initialize();
    updateGUI();
  }

  AbstractEditTab::~AbstractEditTab()
  {
  }

  void AbstractEditTab::updateGUI()
  {
    if (!m_isInitialized) {
      return;
    }
    Q_ASSERT_X(m_queueInterfaces.contains(m_opt->queueInterface()) ||
               m_opt->queueInterface() == 0, Q_FUNC_INFO,
               "Current queue interface is unknown to AbstractEditTab.");
    Q_ASSERT_X(m_optimizers.contains(m_opt->optimizer()) ||
               m_opt->optimizer() == 0, Q_FUNC_INFO,
               "Current optimizer is unknown to AbstractEditTab.");
    Q_ASSERT(m_optimizers.size() == ui_combo_optimizers->count());
    Q_ASSERT(m_queueInterfaces.size() == ui_combo_queueInterfaces->count());

    if (m_opt->optimizer()) {
      int optIndex = m_optimizers.indexOf(m_opt->optimizer());
      ui_combo_optimizers->setCurrentIndex(optIndex);
    }

    if (m_opt->queueInterface()) {
      int qiIndex = m_queueInterfaces.indexOf(m_opt->queueInterface());
      ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
      if (m_opt->queueInterface()->hasDialog()) {
        ui_push_queueInterfaceConfig->setEnabled(true);
      } else {
        ui_push_queueInterfaceConfig->setEnabled(false);
      }
    }
    else {
      ui_push_queueInterfaceConfig->setEnabled(false);
    }

    populateTemplates();
    populateOptStepList();

    updateEditWidget();

    ui_edit_user1->setText(	m_opt->optimizer()->getUser1());
    ui_edit_user2->setText(	m_opt->optimizer()->getUser2());
    ui_edit_user3->setText(	m_opt->optimizer()->getUser3());
    ui_edit_user4->setText(	m_opt->optimizer()->getUser4());
  }

  void AbstractEditTab::lockGUI()
  {
    ui_combo_optimizers->setDisabled(true);
    ui_combo_queueInterfaces->setDisabled(true);
  }

  void AbstractEditTab::showHelp()
  {
    QMessageBox::information(m_dialog,
                             "Template Help",
                             m_opt->getTemplateKeywordHelp());
  }

  void AbstractEditTab::updateQueueInterface()
  {
    Q_ASSERT_X(m_queueInterfaces.contains(m_opt->queueInterface()) ||
               m_opt->queueInterface() == 0, Q_FUNC_INFO,
               "Current queue interface is unknown to AbstractEditTab.");

    unsigned int newQiIndex = ui_combo_queueInterfaces->currentIndex();

    Q_ASSERT(newQiIndex <= m_queueInterfaces.size() - 1);

    // Check that queueInterface has actually changed
    if (m_queueInterfaces.indexOf(m_opt->queueInterface()) ==
        newQiIndex &&
        m_opt->queueInterface() != 0) {
      return;
    }

    QueueInterface *qi = m_queueInterfaces[newQiIndex];

    if (qi->hasDialog()) {
      ui_push_queueInterfaceConfig->setEnabled(true);
    } else {
      ui_push_queueInterfaceConfig->setEnabled(false);
    }

    emit queueInterfaceChanged(qi->getIDString().toLower().toStdString());
  }

  void AbstractEditTab::updateOptimizer()
  {
    Q_ASSERT_X(m_optimizers.contains(m_opt->optimizer()) ||
               m_opt->optimizer() == 0, Q_FUNC_INFO,
               "Current optimizer is unknown to AbstractEditTab.");

    unsigned int newOptimizerIndex = ui_combo_optimizers->currentIndex();

    Q_ASSERT(newOptimizerIndex <= m_optimizers.size() - 1);

    // Check that optimizer has actually changed
    if (m_optimizers.indexOf(m_opt->optimizer()) ==
        newOptimizerIndex &&
        m_opt->optimizer() != 0) {
      return;
    }

    Optimizer *o = m_optimizers[newOptimizerIndex];

    if (o->hasDialog()) {
      ui_push_optimizerConfig->setEnabled(true);
    } else {
      ui_push_optimizerConfig->setEnabled(false);
    }

    emit optimizerChanged(o->getIDString().toLower().toStdString());
  }

  void AbstractEditTab::configureQueueInterface()
  {
    Q_ASSERT(m_opt->queueInterface());
    Q_ASSERT(m_opt->queueInterface()->hasDialog());

    QDialog *d = m_opt->queueInterface()->dialog();
    Q_ASSERT(d != 0);

    d->show();
    d->exec();
  }

  void AbstractEditTab::configureOptimizer()
  {
    Q_ASSERT(m_opt->optimizer());
    Q_ASSERT(m_opt->optimizer()->hasDialog());

    QDialog *d = m_opt->optimizer()->dialog();
    Q_ASSERT(d != 0);

    d->show();
    d->exec();
  }

  QStringList AbstractEditTab::getTemplateNames(size_t optStep)
  {
    if (!m_isInitialized) {
      return QStringList();
    }
    QStringList templateNames =
      m_opt->optimizer(optStep)->getTemplateFileNames();
    templateNames.append(
      m_opt->queueInterface(optStep)->getTemplateFileNames()
    );
    qSort(templateNames);
    return templateNames;
  }

  void AbstractEditTab::populateTemplates()
  {
    if (!m_isInitialized) {
      return;
    }
    int optStepIndex = getCurrentOptStep();
    ui_combo_templates->blockSignals(true);
    ui_combo_templates->clear();
    ui_combo_templates->insertItems(0, getTemplateNames(optStepIndex));
    ui_combo_templates->blockSignals(false);
    ui_combo_templates->setCurrentIndex(0);
  }

  void AbstractEditTab::updateEditWidget()
  {
    if (!m_isInitialized) {
      return;
    }
    int optStepIndex = getCurrentOptStep();
    QStringList filenames = getTemplateNames(optStepIndex);
    int templateInd = ui_combo_templates->currentIndex();
    QString templateName = ui_combo_templates->currentText();
    Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
    Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

    if (m_opt->getNumOptSteps() != ui_list_optStep->count()) {
      populateOptStepList();
    }

    Q_ASSERT(optStepIndex >= 0 &&
             optStepIndex < m_opt->getNumOptSteps());

    // Display appropriate entry widget. Only text entry is supported
    // by default, reimplement this function in the derived class if
    // list entry is needed and just call
    // AbstractEditTab::updateEditWidget to handle entries
    // requiring text entry.
    ui_list_edit->setVisible(false);
    ui_edit_edit->setVisible(true);

    // Update text edit widget
    Q_ASSERT(getTemplateNames(optStepIndex).contains(templateName));
    QString text = m_opt->getTemplate(optStepIndex, templateName);

    ui_edit_edit->blockSignals(true);
    ui_edit_edit->setText(text);
    ui_edit_edit->setCurrentFont(QFont("Courier"));
    ui_edit_edit->blockSignals(false);
  }

  void AbstractEditTab::saveCurrentTemplate()
  {
    int optStepIndex = getCurrentOptStep();
    QStringList filenames = getTemplateNames(optStepIndex);
    int templateInd = ui_combo_templates->currentIndex();
    QString templateName = ui_combo_templates->currentText();
    Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
    Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

    if (m_opt->getNumOptSteps() != ui_list_optStep->count())
      populateOptStepList();

    int optStepIndex = getCurrentOptStep();//getCurrentOptStep();
    Q_ASSERT(optStepIndex >= 0 && optStepIndex < m_opt->getNumOptSteps());

    // Here we only update from the text edit widget. If any templates
    // are using the list input, reimplement this function in the
    // derived class and handle the lists appropriately.
    QString text = ui_edit_edit->document()->toPlainText();

    m_opt->setTemplate(optStepIndex, templateName.toStdString(),
                       text.toStdString());

    ui_edit_edit->setCurrentFont(QFont("Courier"));
  }

  void AbstractEditTab::updateUserValues()
  {
    m_opt->optimizer()->setUser1(ui_edit_user1->text());
    m_opt->optimizer()->setUser2(ui_edit_user2->text());
    m_opt->optimizer()->setUser3(ui_edit_user3->text());
    m_opt->optimizer()->setUser4(ui_edit_user4->text());
  }

  void AbstractEditTab::populateOptStepList()
  {
    if (!m_isInitialized) {
      return;
    }
    ui_list_optStep->blockSignals(true);
    ui_list_optStep->clear();

    // Just return if either QI or optimizer are missing
    if (!m_opt->optimizer() || !m_opt->queueInterface()) {
      ui_list_optStep->blockSignals(false);
      return;
    }

    int currentOptStep = getCurrentOptStep();
    const int maxSteps = m_opt->getNumOptSteps();
    if (currentOptStep < 0) currentOptStep = 0;
    if (currentOptStep >= maxSteps) currentOptStep = maxSteps - 1;

    for (int i = 1; i <= maxSteps; ++i) {
      ui_list_optStep->addItem(tr("Optimization %1").arg(i));
    }
    ui_list_optStep->blockSignals(false);

    ui_list_optStep->setCurrentRow(currentOptStep);
  }

  void AbstractEditTab::appendOptStep()
  {
    // Reimplement in derived class if Optimizer generic data is needed
    const int maxSteps = m_opt->getNumOptSteps();
    const int currentOptStep = getCurrentOptStep();
    Q_ASSERT(currentOptStep >= 0 && currentOptStep < maxSteps);

    m_opt->appendOptStep();

    populateOptStepList();
  }

  void AbstractEditTab::removeCurrentOptStep()
  {
    // Reimplement in derived class if Optimizer generic data is needed
    const int maxSteps = m_opt->getNumOptSteps();
    const int currentOptStep = getCurrentOptStep();
    Q_ASSERT(currentOptStep >= 0 && currentOptStep < maxSteps);

    m_opt->removeOptStep(currentOptStep);

    populateOptStepList();
  }

  void AbstractEditTab::saveScheme()
  {
    SETTINGS("");
    QString oldFilename = settings->value(m_opt->getIDString().toLower() +
                                       "/edit/schemePath/", "").toString();
    QString filename = QFileDialog::getSaveFileName(nullptr,
                            tr("Save Optimization Scheme as..."),
                            oldFilename, "*.scheme;;*.state;;*.*",
                            nullptr, QFileDialog::DontUseNativeDialog);

    // User canceled
    if (filename.isEmpty())
      return;

    settings->setValue(m_opt->getIDString().toLower() +
                       "/edit/schemePath/", filename);
    writeSettings(filename);
  }

  void AbstractEditTab::loadScheme()
  {
    SETTINGS("");
    QString oldFilename = settings->value(m_opt->getIDString().toLower() +
                                         "/edit/schemePath/", "").toString();
    QString filename = QFileDialog::getOpenFileName(nullptr,
                        tr("Select Optimization Scheme to load..."),
                        oldFilename, "*.scheme;;*.state;;*.*",
                        0, QFileDialog::DontUseNativeDialog);

    // User canceled
    if (filename.isEmpty())
      return;

    settings->setValue(m_opt->getIDString().toLower() +
                       "/edit/schemePath/", filename);
    readSettings(filename);
  }

}
