/*
 *@BEGIN LICENSE
 *
 * PSI4: an ab initio quantum chemistry software package
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * BP-v2RDM: a boundary-point semidefinite solver for variational 2-RDM
 *           computations.
 *
 * Copyright (c) 2014, The Florida State University. All rights reserved.
 * 
 *@END LICENSE
 *
 * This code performs a semidefinite optimization of the electronic
 * energy according to the boundary-point algorithm described in
 * PRL 106, 083001 (2011).
 *
 */
#include <psi4-dec.h>
#include <libparallel/parallel.h>
#include <liboptions/liboptions.h>
#include <libqt/qt.h>

#include<libtrans/integraltransform.h>
#include<libtrans/mospace.h>

#include<libmints/wavefunction.h>
#include<libmints/mints.h>
#include<libmints/vector.h>
#include<libmints/matrix.h>
#include<../bin/fnocc/blas.h>
#include<time.h>

#include<psifiles.h>

#include"v2rdm_solver.h"

#ifdef _OPENMP
    #include<omp.h>
#else
    #define omp_get_wtime() ( (double)clock() / CLOCKS_PER_SEC )
    #define omp_get_max_threads() 1
#endif

using namespace boost;
using namespace psi;
using namespace fnocc;

namespace psi{ namespace v2rdm_casscf{

boost::shared_ptr<Matrix> v2RDMSolver::GetOEI() {
    boost::shared_ptr<MintsHelper> mints(new MintsHelper());
    boost::shared_ptr<Matrix> K1 (new Matrix(mints->so_potential()));
    K1->add(mints->so_kinetic());
    K1->transform(Ca_);
    return K1;
}

boost::shared_ptr<Matrix> v2RDMSolver::ReadOEI() {

    boost::shared_ptr<Matrix> K1 (new Matrix("one-electron integrals",nirrep_,nmopi_,nmopi_));

    long int full = amo_ + nfrzc + nfrzv;
    double * tempoei = (double*)malloc(full*(full+1)/2*sizeof(double));
    memset((void*)tempoei,'\0',full*(full+1)/2*sizeof(double));
    boost::shared_ptr<PSIO> psio(new PSIO());
    psio->open(PSIF_OEI,PSIO_OPEN_OLD);
    psio->read_entry(PSIF_OEI,"MO-basis One-electron Ints",(char*)&tempoei[0],full*(full+1)/2*sizeof(double));
    psio->close(PSIF_OEI,1);
    offset = 0;
    for (int h = 0; h < nirrep_; h++) {
        for (long int i = 0; i < nmopi_[h]; i++) {
            for (long int j = 0; j < nmopi_[h]; j++) {
                K1->pointer(h)[i][j] = tempoei[INDEX(i+offset,j+offset)];
            }
        }
        offset += nmopi_[h];
    }
    free(tempoei);

    return K1;
}

}}