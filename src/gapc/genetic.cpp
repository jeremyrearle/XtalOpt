/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <gapc/genetic.h>

#include <globalsearch/macros.h>

#include <QDebug>

#include <cstdlib>
#include <vector>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace GAPC {

  // helper functions
  inline Matrix3d createRandomRotationMatrix()
  {
    // This function builds a rotation matrix:
    //
    // [ [ tx^2+c txy-sz txz+sy ]
    //   [ txy+sz ty^2+c txy-sx ]
    //   [ txz-sy tyz+sx tz^2+c ] ]
    //
    // where x,y,z is the unit vector along the axis of rotation, and
    //
    // c = cos(theta)
    // s = sin(theta)
    // t = 1-cos(theta)
    //
    // (taken from
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/index.htm)
    //
    // First, generate x,y,z randomly and normalize to get the
    // rotation axis:
    double x = RANDDOUBLE();
    double y = RANDDOUBLE();
    double z = RANDDOUBLE();
    double length = sqrt(x*x+y*y+z*z);
    x /= length;
    y /= length;
    z /= length;

    // Now randomly generate theta (on [0,2*pi]), c, s, and t
    double theta = RANDDOUBLE() * 2*M_PI;
    double c = cos(theta);
    double s = sin(theta);
    double t = 1 - c;

    // Actually build rotation matrix
    Matrix3d m;
    m <<
      t*x*x+c, t*x*y-s*z, t*x*z+s*y,
      t*x*y+s*z, t*y*y+c, t*x*y-s*x,
      t*x*z-s*y, t*y*z+s*x, t*z*z+c;
    return m;
  }


  ProtectedCluster* GAPCGenetic::crossover(ProtectedCluster* pc1,
                                                  ProtectedCluster* pc2)
  {
    // Create rotation matricies to modify the clusters
    Matrix3d xform1 = createRandomRotationMatrix();
    Matrix3d xform2 = createRandomRotationMatrix();

    // Get lists of atoms and coordinates
    pc1->lock()->lockForRead();
    // Save composition for checks later
    QList<QString> atomSymbols  = pc1->getSymbols();
    QList<uint> atomCounts      = pc1->getNumberOfAtomsAlpha();
    QList<Atom*> atomList1      = pc1->atoms();
    QList<Vector3d> coordsList1;
    for (int i = 0; i < atomList1.size(); i++)
      coordsList1.append(*(atomList1.at(i)->pos()));
    pc1->lock()->unlock();

    pc2->lock()->lockForRead();
    QList<Atom*> atomList2 = pc2->atoms();
    QList<Vector3d> coordsList2;
    for (int i = 0; i < atomList2.size(); i++)
      coordsList2.append(*(atomList2.at(i)->pos()));
    pc2->lock()->unlock();

    // Transform atoms
    // Eigen::Vector3d is a column vector, so transpose before and
    // after transforming them.
    for (int i = 0; i < coordsList1.size(); i++) {
      coordsList1[i] = (coordsList1[i].transpose() *
                        xform1).transpose();
      coordsList2[i] = (coordsList2[i].transpose() *
                        xform2).transpose();
    }

    // Build new cluster
    ProtectedCluster *npc = new ProtectedCluster();
    QWriteLocker npcLocker (npc->lock());

    // Cut pcs and populate new one.
    for (int i = 0; i < coordsList1.size(); i++) {
      if ( coordsList1.at(i)[0] <= 0.0 ) {
        Atom* newAtom = npc->addAtom();
        newAtom->setAtomicNumber(atomList1.at(i)->atomicNumber());
        newAtom->setPos(coordsList1.at(i));
      }
      if ( coordsList2.at(i)[0] > 0.0 ) {
        Atom* newAtom = npc->addAtom();
        newAtom->setAtomicNumber(atomList2.at(i)->atomicNumber());
        newAtom->setPos(coordsList2.at(i));
      }
    }

    // Check composition of npc
    QList<int> deltas;
    QList<QString> nAtomSymbols = npc->getSymbols();
    QList<uint> nAtomCounts   = npc->getNumberOfAtomsAlpha();
    // Fill in 0's for any missing atom types in npc
    if (atomSymbols != nAtomSymbols) {
      for (int i = 0; i < atomSymbols.size(); i++) {
        if (i >= nAtomSymbols.size())
          nAtomSymbols.append("");
        if (atomSymbols.at(i) != nAtomSymbols.at(i)) {
          nAtomSymbols.insert(i,atomSymbols.at(i));
          nAtomCounts.insert(i, 0);
        }
      }
    }
    // Get differences --
    // a (+) value in deltas indicates that more atoms are needed in npc
    // a (-) value indicates less are needed.
    for (int i = 0; i < atomCounts.size(); i++)
      deltas.append(atomCounts.at(i) - nAtomCounts.at(i));
    // Correct for differences by inserting atoms from
    // discarded portions of parents or removing random atoms.
    int delta, atomicNumber;
    for (int i = 0; i < deltas.size(); i++) {
      delta = deltas.at(i);
      atomicNumber = OpenBabel::etab.GetAtomicNum(atomSymbols.at(i).toStdString().c_str());
      // qDebug() << "Delta = " << delta;
      if (delta == 0) continue;
      while (delta < 0) { //qDebug() << "Too many " << atomSymbols.at(i) << "!";
        // Randomly delete atoms from npc;
        // 1 in X chance of each atom being deleted, where
        // X is the total number of that atom type in npc.
        QList<Atom*> atomList = npc->atoms();
        for (int j = 0; j < atomList.size(); j++) {
          if (atomList.at(j)->atomicNumber() == atomicNumber) {
            // atom at j is the type that needs to be deleted.
            if (RANDDOUBLE() < 1.0/static_cast<double>(nAtomCounts.at(i))) {
              // If the odds are right, delete the atom and break loop to recheck condition.
              npc->removeAtom(atomList.at(j)); // removeAtom(Atom*) takes care of deleting pointer.
              delta++;
              break;
            }
          }
        }
      }
      while (delta > 0) { //qDebug() << "Too few " << atomSymbols.at(i) << "!";
        // Randomly add atoms from discarded cuts of parent pcs;
        // 1 in X chance of each atom being added, where
        // X is the total number of atoms of that species in the parent.
        //
        // First, pick the parent. 50/50 chance for each:
        uint parent;
        if (RANDDOUBLE() < 0.5) parent = 1;
        else parent = 2;
        for (int j = 0; j < coordsList1.size(); j++) { // size should be the same for both parents
          if (
              // if atom at j is the type that needs to be added,
              (
               ( parent == 1 && atomList1.at(j)->atomicNumber() == atomicNumber)
               ||
               ( parent == 2 && atomList2.at(j)->atomicNumber() == atomicNumber)
              )
              &&
              // and atom is in the discarded region of the cut,
              (
               ( parent == 1 && coordsList1.at(j)[0] > 0.0 )
               ||
               ( parent == 2 && coordsList2.at(j)[0] <= 0.0)
               )
              &&
              // and the odds favor it, add the atom to npc
              ( RANDDOUBLE() < 1.0/static_cast<double>(atomCounts.at(i)) )
              ) {
            Atom* newAtom = npc->addAtom();
            newAtom->setAtomicNumber(atomicNumber);
            if ( parent == 1)
              newAtom->setPos(coordsList1.at(j));
            else // ( parent == 2)
              newAtom->setPos(coordsList2.at(j));
            delta--;
            break;
          }
        }
      }
    }

    // Done!
    npc->centerAtoms();
    npc->setStatus(ProtectedCluster::WaitingForOptimization);
    return npc;
  }

  ProtectedCluster* GAPCGenetic::twist(ProtectedCluster* pc,
                                       float minimumRotation,
                                       float &rotationDeg)
  {

  }

  ProtectedCluster* GAPCGenetic::exchange(ProtectedCluster* pc,
                                          unsigned int exchanges)
  {

  }

  ProtectedCluster* GAPCGenetic::randomWalk(ProtectedCluster* pc,
                                            unsigned int numberAtoms,
                                            float minWalk,
                                            float maxWalk)
  {

  }

}
