/**********************************************************************
  VASPOptimizer - Tools to interface with VASP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/structures/xtal.h>

#include <globalsearch/macros.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/sshmanager.h>

#include <QDir>
#include <QDebug>
#include <QString>
#include <QSettings>

using namespace GlobalSearch;

namespace XtalOpt {

  VASPOptimizer::VASPOptimizer(OptBase *parent, const QString &filename) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs
    m_data.insert("POTCAR info",QVariant());
    m_data.insert("Composition",QVariant());

    // Set allowed filenames, e.g.
    m_templates.insert("INCAR",QStringList(""));
    m_templates.insert("POTCAR",QStringList(""));
    m_templates.insert("KPOINTS",QStringList(""));

    // Setup for completion values
    m_completionFilename = "OUTCAR";
    m_completionStrings.clear();
    m_completionStrings.append("General timing and accounting informations for this job:");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append("CONTCAR");
    m_outputFilenames.append("POSCAR");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "VASP";

    // Local execution setup:
    m_localRunCommand = "vasp";
    m_stdinFilename = "";
    m_stdoutFilename = "";
    m_stderrFilename = "";

    readSettings(filename);

    buildPOTCARs();
  }

  void VASPOptimizer::readSettings(const QString &filename)
  {
    // Don't consider default setting,, only schemes and states.
    if (filename.isEmpty())
      return;

    readTemplatesFromSettings(filename);
    readUserValuesFromSettings(filename);
    readDataFromSettings(filename);

    buildPOTCARs();
  }

  void VASPOptimizer::writeTemplatesToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      // Don't bother saving the actual POTCAR files
      if (filenames.at(i) == "POTCAR") continue;
      settings->setValue("xtalopt/optimizer/" +
                    getIDString() + "/" +
                    filenames.at(i) + "_list",
                    m_templates.value(filenames.at(i)));
    }

    // QueueInterface templates
    settings->beginGroup(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() + "/QI/" +
                         m_opt->queueInterface()->getIDString() + "/");
    filenames = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      settings->setValue((*it) + "_list",
                         m_QITemplates.value(*it));
    }
    settings->endGroup();
  }

  void VASPOptimizer::writeDataToSettings(const QString &filename)
  {
    // We only want to save POTCAR info and Composition to the resume
    // file, not the main config file, so only dump the data here if
    // we are given a filename and it contains the string
    // "xtalopt.state"
    if (filename.isEmpty() || !filename.contains("xtalopt.state")) {
      return;
    }
    SETTINGS(filename);
    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      settings->setValue("xtalopt/optimizer/" +
                         getIDString() + "/data/" +
                         ids.at(i),
                         m_data.value(ids.at(i)));
    }
  }

  QHash<QString, QString>
  VASPOptimizer::getInterpretedTemplates(Structure *structure)
  {
    QHash<QString, QString> hash = Optimizer::getInterpretedTemplates(structure);
    hash.insert("POSCAR", m_opt->interpretTemplate("%POSCAR%", structure));
    return hash;
  }

  bool VASPOptimizer::POTCARInfoIsUpToDate(QList<uint> atomicNums)
  {
    // Get optimizer's composition
    QList<uint> oldcomp;
    QList<QVariant> oldcomp_ = getData("Composition").toList();
    for (int i = 0; i < oldcomp_.size(); i++)
      oldcomp.append(oldcomp_.at(i).toUInt());
    // Sort the composition
    qSort(atomicNums);
    qSort(oldcomp);

    if (getData("POTCAR info").toList().size() == 0 || oldcomp != atomicNums)
      return false;

    if (getData("POTCAR info").toList().size() != getNumberOfOptSteps()) {
      // Add or take away POTCAR info items to match the number of opt steps.
      QVariantList potcarInfo = getData("POTCAR info").toList();
      // potcarInfo.size() must be greater than 0 here. The previous if
      // statement would have caused a return if potcarInfo.size() was 0.
      while (potcarInfo.size() < getNumberOfOptSteps())
        potcarInfo.append(potcarInfo[0]);
      while (potcarInfo.size() > getNumberOfOptSteps())
        potcarInfo.pop_back();
      setData("POTCAR info", potcarInfo);
    }
    return true;
  }

  void VASPOptimizer::buildPOTCARs() {
    double enmax = 0;

    int numOptSteps = getNumberOfOptSteps();

    m_templates["POTCAR"].clear();
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs

    // Make sure this doesn't exceed the number of opt steps
    QVariantList potcarInfo = m_data["POTCAR info"].toList();
    while (potcarInfo.size() > numOptSteps)
      potcarInfo.pop_back();
    setData("POTCAR info", potcarInfo);

    for (int optIndex = 0;
         optIndex < potcarInfo.size(); optIndex++) {
      QFile file;
      double tmp_enmax;
      QString line, str, POTCAR;
      QStringList tmp_sl;
      QTextStream out (&POTCAR), in;
      QList<QString> symbols = potcarInfo.at(optIndex).toHash().keys();
      qSort(symbols);
      // Make a loop over the alphabetically sorted symbols:
      for (int i = 0; i < symbols.size(); i++) {
        file.setFileName(potcarInfo.at(optIndex).toHash().value(symbols.at(i)).toString());
        file.open(QIODevice::ReadOnly);
        in.setDevice(&file);
        while (!in.atEnd()) {
          line = in.readLine();
          out << line + "\n";
          if (line.contains("ENMAX")) {
            tmp_sl = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            str = tmp_sl.at(2);
            str.remove(";");
            tmp_enmax = str.toFloat();
            if (tmp_enmax > enmax) enmax = tmp_enmax;
          }
        }
        file.close();
      }
      appendTemplate("POTCAR", POTCAR);
    }
    // BUG FIX - m_templates["POTCAR"] must at least have an empty string in it
    if (m_templates["POTCAR"].isEmpty())
      m_templates["POTCAR"] = QStringList("");
  }

} // end namespace XtalOpt
