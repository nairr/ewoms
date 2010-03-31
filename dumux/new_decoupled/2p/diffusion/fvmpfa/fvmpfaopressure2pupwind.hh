// $Id$
/*****************************************************************************
 *   Copyright (C) 2010 by Markus Wolff                                      *
 *   Copyright (C) 2007-2010 by Yufei Cao                                    *
 *   Institute of Applied Analysis and Numerical Simulation                  *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@mathematik.uni-stuttgart.de                   *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
#ifndef DUNE_FVMPFAOPRESSURE2PUPWIND_HH
#define DUNE_FVMPFAOPRESSURE2PUPWIND_HH

// dune environent:
#include <dune/istl/bvector.hh>
#include <dune/istl/operators.hh>
#include <dune/istl/solvers.hh>
#include <dune/istl/preconditioners.hh>

// dumux environment
#include "dumux/pardiso/pardiso.hh"
#include <dumux/new_decoupled/2p/2pproperties.hh>
#include <dumux/new_decoupled/2p/diffusion/fvmpfa/fvmpfaovelocity2p.hh>
#include <dumux/new_decoupled/2p/diffusion/fvmpfa/mpfaproperties.hh>

/**
 * @file
 * @brief  Base class for defining an instance of a numerical diffusion model
 * @brief  MPFA O-method
 * @brief  Remark1: only for 2-D quadrilateral grid.
 * @brief  Remark2: can use UGGrid or SGrid (YaspGrid); variable 'ch' is chosen to decide which grid will be used.
 * @brief  Remark3: without capillary pressure and gravity!
 * @author Yufei Cao
 */

namespace Dumux
{
//! \ingroup diffusion
//! Base class for defining an instance of a numerical diffusion model.
/*! An interface for defining a numerical diffusion model for the
 *  solution of equations of the form
 * \f$ - \text{div}\, (\lambda K \text{grad}\, p ) = 0, \f$,
 * \f$p = g\f$ on \f$\Gamma_1\f$, and
 * \f$-\lambda K \text{grad}\, p \cdot \mathbf{n} = J\f$
 * on \f$\Gamma_2\f$. Here,
 * \f$p\f$ denotes the pressure, \f$K\f$ the absolute permeability,
 * and \f$\lambda\f$ the total mobility, possibly depending on the
 * saturation.
 Template parameters are:

 - GridView      a DUNE gridView type
 - Scalar        type used for return values
 */
template<class TypeTag>
class FVMPFAOPressure2PUpwind
{
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Problem)) Problem;
    typedef typename GET_PROP(TypeTag, PTAG(ReferenceElements)) ReferenceElements;
    typedef typename ReferenceElements::Container ReferenceElementContainer;
    typedef typename ReferenceElements::ContainerFaces ReferenceElementFaceContainer;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(SpatialParameters)) SpatialParameters;
    typedef typename SpatialParameters::MaterialLaw MaterialLaw;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(TwoPIndices)) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FluidSystem)) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FluidState)) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridTypeIndices)) GridTypeIndices;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };
    enum
    {
        Sw = Indices::saturationW,
        Sn = Indices::saturationNW,
        vw = Indices::velocityW,
        vn = Indices::velocityNW,
        vt = Indices::velocityTotal
    };
    enum
    {
        wPhaseIdx = Indices::wPhaseIdx, nPhaseIdx = Indices::nPhaseIdx
    };

    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::Grid Grid;
    typedef typename GridView::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::IntersectionIterator IntersectionIterator;

    typedef Dune::FieldVector<Scalar, dim> LocalPosition;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::FieldMatrix<Scalar, dim, dim> FieldMatrix;
    typedef Dune::FieldVector<Scalar, dim> FieldVector;

    typedef Dune::FieldMatrix<Scalar, 1, 1> MB;
    typedef Dune::BCRSMatrix<MB> Matrix;
    typedef Dune::BlockVector<Dune::FieldVector<Scalar, 1> > Vector;

    //initializes the matrix to store the system of equations
    void initializeMatrix();

    //function which assembles the system of equations to be solved
    void assemble();

    //solves the system of equations to get the spatial distribution of the pressure
    void solve();

protected:
    Problem& problem()
    {
        return problem_;
    }

    const Problem& problem() const
    {
        return problem_;
    }

public:

    //constitutive functions are initialized and stored in the variables object
    void updateMaterialLaws(bool first);

    void initial(bool solveTwice = true)
    {
        updateMaterialLaws(true);

        FVMPFAOVelocity2P<TypeTag> firstVelocity(problem_);
        firstVelocity.pressure();
        firstVelocity.calculateVelocity();

        updateMaterialLaws(false);

        assemble();
        solve();

        return;
    }

    // serialization methods
    template<class Restarter>
    void serialize(Restarter &res)
    {
        return;
    }

    template<class Restarter>
    void deserialize(Restarter &res)
    {
        return;
    }

    //! \brief Write data files
    /*  \param name file name */
    template<class MultiWriter>
    void addOutputVtkFields(MultiWriter &writer)
    {
        problem().variables().addOutputVtkFields(writer);
        return;
    }

    void pressure(bool solveTwice = true)
    {
        //        Dune::Timer timer;

        //        timer.reset();
        assemble();
        //        std::cout << "assembling MPFA O-matrix on level" << problem_.gridView().grid().maxLevel() << " took " << timer.elapsed() << " seconds" << std::endl;

        //        timer.reset();
        solve();
        //        std::cout << "solving MPFA O-matrix on level" << problem_.gridView().grid().maxLevel() << " took " << timer.elapsed() << " seconds" << std::endl;

        return;
    }

    FVMPFAOPressure2PUpwind(Problem& problem) :
        problem_(problem), M_(problem.variables().gridSize(), problem.variables().gridSize(), (4 * dim + (dim - 1))
                * problem.variables().gridSize(), Dune::BCRSMatrix<MB>::random), f_(problem.variables().gridSize()),
                solverName_("BiCGSTAB"), preconditionerName_("SeqILU0")
    {
        initializeMatrix();
    }

    FVMPFAOPressure2PUpwind(Problem& problem, std::string solverName, std::string preconditionerName) :
        problem_(problem), M_(problem.variables().gridSize(), problem.variables().gridSize(), (4 * dim + (dim - 1))
                * problem.variables().gridSize(), Dune::BCRSMatrix<MB>::random), f_(problem.variables().gridSize()),
                solverName_(solverName), preconditionerName_(preconditionerName)
    {
        initializeMatrix();
    }

private:
    Problem& problem_;
    Matrix M_;
    Vector f_;
    std::string solverName_;
    std::string preconditionerName_;
protected:
    static const int saturationType = GET_PROP_VALUE(TypeTag, PTAG(SaturationFormulation)); //!< gives kind of saturation used (\f$ 0 = S_w\f$, \f$ 1 = S_n\f$)
    static const int velocityType_ = GET_PROP_VALUE(TypeTag, PTAG(VelocityFormulation)); //!< gives kind of velocity used (\f$ 0 = v_w\f$, \f$ 1 = v_n\f$, \f$ 2 = v_t\f$)

};

template<class TypeTag>
void FVMPFAOPressure2PUpwind<TypeTag>::initializeMatrix()
{
    // determine matrix row sizes
    ElementIterator eItBegin = problem_.gridView().template begin<0> ();
    ElementIterator eItEnd = problem_.gridView().template end<0> ();
    for (ElementIterator eIt = eItBegin; eIt != eItEnd; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        // initialize row size
        int rowSize = 1;

        // run through all intersections with neighbors
        IntersectionIterator isItBegin = problem_.gridView().template ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().template iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            // 'nextisIt' iterates over next codimension 1 intersection neighboring with 'isIt'
            IntersectionIterator nextisIt = ++tempisIt;

            // get 'nextisIt'
            switch (GET_PROP_VALUE(TypeTag, PTAG(GridImplementation)))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextisIt == isItEnd)
                    nextisIt = isItBegin;
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
            // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextisIt == isItEnd)
                    nextisIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
            }
            }

            if (isIt->neighbor())
                rowSize++;

            if (isIt->neighbor() && nextisIt->neighbor())
                rowSize++;
        } // end of 'for' IntersectionIterator

        // set number of indices in row globalIdxI to rowSize
        M_.setrowsize(globalIdxI, rowSize);

    } // end of 'for' ElementIterator

    // indicate that size of all rows is defined
    M_.endrowsizes();

    // determine position of matrix entries
    for (ElementIterator eIt = eItBegin; eIt != eItEnd; ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        // add diagonal index
        M_.addindex(globalIdxI, globalIdxI);

        // run through all intersections with neighbors
        IntersectionIterator isItBegin = problem_.gridView().template ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().template iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            // 'nextisIt' iterates over next codimension 1 intersection neighboring with 'isIt'
            // sequence of next is anticlockwise of 'isIt'
            IntersectionIterator nextisIt = ++tempisIt;

            // get 'nextisIt'
            switch (GET_PROP_VALUE(TypeTag, PTAG(GridImplementation)))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
            // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextisIt == isItEnd)
                    nextisIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
            }
            }

            if (isIt->neighbor())
            {
                // access neighbor
                ElementPointer outside = isIt->outside();
                int globalIdxJ = problem_.variables().index(*outside);

                // add off diagonal index
                // add index (row,col) to the matrix
                M_.addindex(globalIdxI, globalIdxJ);
            }

            if (isIt->neighbor() && nextisIt->neighbor())
            {
                // access the common neighbor of isIt's and nextisIt's outside
                ElementPointer outside = isIt->outside();
                ElementPointer nextisItoutside = nextisIt->outside();

                IntersectionIterator innerisItEnd = problem_.gridView().template iend(*outside);
                IntersectionIterator innernextisItEnd = problem_.gridView().template iend(*nextisItoutside);

                for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*outside); innerisIt
                        != innerisItEnd; ++innerisIt)
                    for (IntersectionIterator innernextisIt = problem_.gridView().template ibegin(*nextisItoutside); innernextisIt
                            != innernextisItEnd; ++innernextisIt)
                    {
                        if (innerisIt->neighbor() && innernextisIt->neighbor())
                        {
                            ElementPointer innerisItoutside = innerisIt->outside();
                            ElementPointer innernextisItoutside = innernextisIt->outside();

                            if (innerisItoutside == innernextisItoutside && innerisItoutside != isIt->inside())
                            {
                                int globalIdxJ = problem_.variables().index(*innerisItoutside);

                                M_.addindex(globalIdxI, globalIdxJ);
                            }
                        }
                    }
            }
        } // end of 'for' IntersectionIterator
    } // end of 'for' ElementIterator

    // indicate that all indices are defined, check consistency
    M_.endindices();

    return;
}

// only for 2-D general quadrilateral
template<class TypeTag>
void FVMPFAOPressure2PUpwind<TypeTag>::assemble()
{
    // initialization: set global matrix M_ to zero
    M_ = 0;

    // introduce matrix R for vector rotation and R is initialized as zero matrix
    FieldMatrix R(0);

    // evaluate matrix R
    if (dim == 2)
        for (int i = 0; i < dim; ++i)
        {
            R[0][1] = 1;
            R[1][0] = -1;
        }

    // run through all elements
    ElementIterator eItEnd = problem_.gridView().template end<0> ();
    for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != eItEnd; ++eIt)
    {
        // get common geometry information for the following computation

        // cell 1 geometry type
        Dune::GeometryType gt1 = eIt->geometry().type();

        // get global coordinate of cell 1 center
        GlobalPosition globalPos1 = eIt->geometry().center();

        // cell 1 volume
        Scalar volume1 = eIt->geometry().volume();

        // cell 1 index
        int globalIdx1 = problem_.variables().index(*eIt);

        // evaluate right hand side
        std::vector<Scalar> source(problem_.source(globalPos1, *eIt));
        f_[globalIdx1] = volume1 * (source[wPhaseIdx] + source[nPhaseIdx]);

        // get absolute permeability of cell 1
        FieldMatrix K1(problem_.spatialParameters().intrinsicPermeability(globalPos1, *eIt));

        //get the densities
        Scalar densityW = this->problem().variables().densityWetting(globalIdx1);
        Scalar densityNW = this->problem().variables().densityNonwetting(globalIdx1);

        // if K1 is zero, no flux through cell1
        // for 2-D
        if (K1[0][0] == 0 && K1[0][1] == 0 && K1[1][0] == 0 && K1[1][1] == 0)
        {
            M_[globalIdx1][globalIdx1] += 1.0;
            continue;
        }

        IntersectionIterator isItBegin = problem_.gridView().template ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().template iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            // intersection iterator 'nextisIt' is used to get geometry information
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            IntersectionIterator nextisIt = ++tempisIt;

            //get nextisIt
            switch (GET_PROP_VALUE(TypeTag, PTAG(GridImplementation)))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
            // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextisIt == isItEnd)
                    nextisIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
            }
            }

            // get local number of facet 'isIt'
            int indexInInside = isIt->indexInInside();

            //compute total mobility of cell 1
            Scalar lambda1 = problem_.variables().upwindMobilitiesWetting(globalIdx1, indexInInside, 0)
                    + problem_.variables().upwindMobilitiesNonwetting(globalIdx1, indexInInside, 0);


            // get geometry type of face 'isIt', i.e., the face between cell1 and cell2 (locally numbered)
            Dune::GeometryType gtf12 = isIt->geometryInInside().type();

            // center of face in global coordinates, i.e., the midpoint of edge 'isIt'
            GlobalPosition globalPosFace12 = isIt->geometry().center();

            // get face volume
            Scalar face12vol = isIt->geometry().volume();

            // get outer normal vector scaled with half volume of face 'isIt'
            Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln1 = isIt->centerUnitOuterNormal();
            integrationOuterNormaln1 *= face12vol / 2.0;

            // get geometry type of 'nextisIt', i.e., face between cell1 and cell3 (locally numbered)
            Dune::GeometryType gtf13 = nextisIt->geometryInInside().type();

            // center of face in global coordinates, i.e., the midpoint of edge 'nextisIt'
            GlobalPosition globalPosFace13 = nextisIt->geometry().center();

            // get local number of facet 'nextisIt'
            int nextIndexInInside = nextisIt->indexInInside();

            // get face volume
            Scalar face13vol = nextisIt->geometry().volume();

            // get outer normal vector scaled with half volume of face 'nextisIt'
            Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln3 = nextisIt->centerUnitOuterNormal();
            integrationOuterNormaln3 *= face13vol / 2.0;

            // get the intersection node /bar^{x_3} between 'isIt' and 'nextisIt', denoted as 'corner1234'
            // initialization of corner1234
            GlobalPosition corner1234(0);

            // get the global coordinate of corner1234
            for (int i = 0; i < isIt->geometry().corners(); ++i)
            {
                GlobalPosition isItcorner = isIt->geometry().corner(i);

                for (int j = 0; j < nextisIt->geometry().corners(); ++j)
                {
                    GlobalPosition nextisItcorner = nextisIt->geometry().corner(j);

                    if (nextisItcorner == isItcorner)
                    {
                        corner1234 = isItcorner;
                        continue;
                    }
                }
            }

            // get total mobility of neighbor cell 2
            Scalar lambda2 = problem_.variables().upwindMobilitiesWetting(globalIdx1, indexInInside, 1)
                    + problem_.variables().upwindMobilitiesNonwetting(globalIdx1, indexInInside, 1);

            // get total mobility of neighbor cell 3
            Scalar lambda3 = problem_.variables().upwindMobilitiesWetting(globalIdx1, indexInInside, 2)
                    + problem_.variables().upwindMobilitiesNonwetting(globalIdx1, indexInInside, 2);


            // handle interior face
            if (isIt->neighbor())
            {
                // access neighbor cell 2 of 'isIt'
                ElementPointer outside = isIt->outside();
                int globalIdx2 = this->problem().variables().index(*outside);

                int indexInInside2 = isIt->indexInOutside();

                // neighbor cell 2 geometry type
                Dune::GeometryType gt2 = outside->geometry().type();

                // get global coordinate of neighbor cell 2 center
                GlobalPosition globalPos2 = outside->geometry().center();

                // get absolute permeability of neighbor cell 2
                FieldMatrix K2(problem_.spatialParameters().intrinsicPermeability(globalPos2, *outside));

                // 'nextisIt' is an interior face
                if (nextisIt->neighbor())
                {
                    // neighbor cell 3
                    // access neighbor cell 3
                    ElementPointer nextisItoutside = nextisIt->outside();
                    int globalIdx3 = this->problem().variables().index(*nextisItoutside);

                    int indexInInside3 = nextisIt->indexInOutside();

                    // get basic information of cell 1,2's neighbor cell 3,4
                    // neighbor cell 3
                    // access neighbor cell 3
                    // neighbor cell 3 geometry type
                    Dune::GeometryType gt3 = nextisItoutside->geometry().type();

                    // get global coordinate of neighbor cell 3 center
                    GlobalPosition globalPos3 = nextisItoutside->geometry().center();

                    // get absolute permeability of neighbor cell 3
                    FieldMatrix K3(problem_.spatialParameters().intrinsicPermeability(globalPos3, *nextisItoutside));

                    // neighbor cell 4
                    GlobalPosition globalPos4(0);
                    FieldMatrix K4(0);
                    Scalar lambda4 = 0;
                    int globalIdx4 = 0;

                    IntersectionIterator innerisItEnd = problem_.gridView().template iend(*outside);
                    IntersectionIterator innernextisItEnd = problem_.gridView().template iend(*nextisItoutside);
                    for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*outside); innerisIt
                            != innerisItEnd; ++innerisIt)
                        for (IntersectionIterator innernextisIt = problem_.gridView().template ibegin(*nextisItoutside); innernextisIt
                                != innernextisItEnd; ++innernextisIt)
                        {
                            if (innerisIt->neighbor() && innernextisIt->neighbor())
                            {
                                ElementPointer innerisItoutside = innerisIt->outside();
                                ElementPointer innernextisItoutside = innernextisIt->outside();

                                // find the common neighbor cell between cell 2 and cell 3, except cell 1
                                if (innerisItoutside == innernextisItoutside && innerisItoutside != isIt->inside())
                                {
                                    int indexInInside4 = innerisIt->indexInOutside();
                                    int nextIndexInInside4 = innernextisIt->indexInOutside();

                                    int indexInOutside4 = innerisIt->indexInInside();
                                    int nextindexInOutside4 = innernextisIt->indexInInside();

                                    // access neighbor cell 4
                                    globalIdx4 = problem_.variables().index(*innerisItoutside);

                                    // neighbor cell 4 geometry type
                                    Dune::GeometryType gt4 = innerisItoutside->geometry().type();

                                    // get global coordinate of neighbor cell 4 center
                                    globalPos4 = innerisItoutside->geometry().center();

                                    // get absolute permeability of neighbor cell 4
                                    K4 += problem_.spatialParameters().intrinsicPermeability(globalPos4,
                                            *innerisItoutside);

                                    // get total mobility of neighbor cell 4
                                    lambda4 = problem_.variables().upwindMobilitiesWetting(globalIdx1, indexInInside, 3)
                                                    + problem_.variables().upwindMobilitiesNonwetting(globalIdx1,
                                                            indexInInside, 3);


                                }
                            }
                        }

                    // computation of flux through the first half edge of 'isIt' and the flux
                    // through the second half edge of 'nextisIt'

                    // get the information of the face 'isIt24' between cell2 and cell4 (locally numbered)
                    IntersectionIterator isIt24 = problem_.gridView().template ibegin(*outside);

                    for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*outside); innerisIt
                            != innerisItEnd; ++innerisIt)
                    {
                        if (innerisIt->neighbor())
                        {
                            if (innerisIt->outside() != isIt->inside())
                            {
                                for (int i = 0; i < innerisIt->geometry().corners(); ++i)
                                {
                                    GlobalPosition innerisItcorner = innerisIt->geometry().corner(i);

                                    if (innerisItcorner == corner1234)
                                    {
                                        isIt24 = innerisIt;
                                        continue;
                                    }
                                }
                            }
                        }
                    }

                    // get geometry type of face 'isIt24'
                    Dune::GeometryType gtf24 = isIt24->geometryInInside().type();

                    // center of face in global coordinates, i.e., the midpoint of edge 'isIt24'
                    GlobalPosition globalPosFace24 = isIt24->geometry().center();

                    // get face volume
                    Scalar face24vol = isIt24->geometry().volume();

                    // get outer normal vector scaled with half volume of face 'isIt24'
                    Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln4 = isIt24->centerUnitOuterNormal();
                    integrationOuterNormaln4 *= face24vol / 2.0;

                    // get the information of the face 'isIt34' between cell3 and cell4 (locally numbered)
                    IntersectionIterator isIt34 = problem_.gridView().template ibegin(*nextisItoutside);

                    for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*nextisItoutside); innerisIt
                            != innernextisItEnd; ++innerisIt)
                    {
                        if (innerisIt->neighbor())
                        {
                            if (innerisIt->outside() != isIt->inside())
                            {
                                for (int i = 0; i < innerisIt->geometry().corners(); ++i)
                                {
                                    GlobalPosition innerisItcorner = innerisIt->geometry().corner(i);

                                    if (innerisItcorner == corner1234)
                                    {
                                        isIt34 = innerisIt;
                                        continue;
                                    }
                                }
                            }
                        }
                    }

                    // get geometry type of face 'isIt34'
                    Dune::GeometryType gtf34 = isIt34->geometryInInside().type();

                    // center of face in global coordinates, i.e., the midpoint of edge 'isIt34'
                    GlobalPosition globalPosFace34 = isIt34->geometry().center();

                    // get face volume
                    Scalar face34vol = isIt34->geometry().volume();

                    // get outer normal vector scaled with half volume of face 'isIt34'
                    Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln2 = isIt34->centerUnitOuterNormal();
                    integrationOuterNormaln2 *= face34vol / 2.0;

                    // compute normal vectors nu11,nu21; nu12, nu22; nu13, nu23; nu14, nu24;
                    FieldVector nu11(0);
                    R.umv(globalPosFace13 - globalPos1, nu11);

                    FieldVector nu21(0);
                    R.umv(globalPos1 - globalPosFace12, nu21);

                    FieldVector nu12(0);
                    R.umv(globalPosFace24 - globalPos2, nu12);

                    FieldVector nu22(0);
                    R.umv(globalPosFace12 - globalPos2, nu22);

                    FieldVector nu13(0);
                    R.umv(globalPos3 - globalPosFace13, nu13);

                    FieldVector nu23(0);
                    R.umv(globalPos3 - globalPosFace34, nu23);

                    FieldVector nu14(0);
                    R.umv(globalPos4 - globalPosFace24, nu14);

                    FieldVector nu24(0);
                    R.umv(globalPosFace34 - globalPos4, nu24);

                    // compute dF1, dF2, dF3, dF4 i.e., the area of quadrilateral made by normal vectors 'nu'
                    FieldVector Rnu21(0);
                    R.umv(nu21, Rnu21);
                    Scalar dF1 = fabs(nu11 * Rnu21);

                    FieldVector Rnu22(0);
                    R.umv(nu22, Rnu22);
                    Scalar dF2 = fabs(nu12 * Rnu22);

                    FieldVector Rnu23(0);
                    R.umv(nu23, Rnu23);
                    Scalar dF3 = fabs(nu13 * Rnu23);

                    FieldVector Rnu24(0);
                    R.umv(nu24, Rnu24);
                    Scalar dF4 = fabs(nu14 * Rnu24);

                    // compute components needed for flux calculation, denoted as 'g'
                    FieldVector K1nu11(0);
                    K1.umv(nu11, K1nu11);
                    FieldVector K1nu21(0);
                    K1.umv(nu21, K1nu21);
                    FieldVector K2nu12(0);
                    K2.umv(nu12, K2nu12);
                    FieldVector K2nu22(0);
                    K2.umv(nu22, K2nu22);
                    FieldVector K3nu13(0);
                    K3.umv(nu13, K3nu13);
                    FieldVector K3nu23(0);
                    K3.umv(nu23, K3nu23);
                    FieldVector K4nu14(0);
                    K4.umv(nu14, K4nu14);
                    FieldVector K4nu24(0);
                    K4.umv(nu24, K4nu24);
                    Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                    Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                    Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                    Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                    Scalar g112 = lambda2 * (integrationOuterNormaln1 * K2nu12) / dF2;
                    Scalar g122 = lambda2 * (integrationOuterNormaln1 * K2nu22) / dF2;
                    Scalar g212 = lambda2 * (integrationOuterNormaln4 * K2nu12) / dF2;
                    Scalar g222 = lambda2 * (integrationOuterNormaln4 * K2nu22) / dF2;
                    Scalar g113 = lambda3 * (integrationOuterNormaln2 * K3nu13) / dF3;
                    Scalar g123 = lambda3 * (integrationOuterNormaln2 * K3nu23) / dF3;
                    Scalar g213 = lambda3 * (integrationOuterNormaln3 * K3nu13) / dF3;
                    Scalar g223 = lambda3 * (integrationOuterNormaln3 * K3nu23) / dF3;
                    Scalar g114 = lambda4 * (integrationOuterNormaln2 * K4nu14) / dF4;
                    Scalar g124 = lambda4 * (integrationOuterNormaln2 * K4nu24) / dF4;
                    Scalar g214 = lambda4 * (integrationOuterNormaln4 * K4nu14) / dF4;
                    Scalar g224 = lambda4 * (integrationOuterNormaln4 * K4nu24) / dF4;

                    // compute transmissibility matrix T = CA^{-1}B+F
                    Dune::FieldMatrix<Scalar, 2 * dim, 2 * dim> C(0), F(0), A(0), B(0);

                    // evaluate matrix C, F, A, B
                    C[0][0] = -g111;
                    C[0][2] = -g121;
                    C[1][1] = g114;
                    C[1][3] = g124;
                    C[2][1] = -g213;
                    C[2][2] = g223;
                    C[3][0] = g212;
                    C[3][3] = -g222;

                    F[0][0] = g111 + g121;
                    F[1][3] = -g114 - g124;
                    F[2][2] = g213 - g223;
                    F[3][1] = -g212 + g222;

                    A[0][0] = g111 + g112;
                    A[0][2] = g121;
                    A[0][3] = -g122;
                    A[1][1] = g114 + g113;
                    A[1][2] = -g123;
                    A[1][3] = g124;
                    A[2][0] = g211;
                    A[2][1] = -g213;
                    A[2][2] = g223 + g221;
                    A[3][0] = -g212;
                    A[3][1] = g214;
                    A[3][3] = g222 + g224;

                    B[0][0] = g111 + g121;
                    B[0][1] = g112 - g122;
                    B[1][2] = g113 - g123;
                    B[1][3] = g114 + g124;
                    B[2][0] = g211 + g221;
                    B[2][2] = -g213 + g223;
                    B[3][1] = -g212 + g222;
                    B[3][3] = g214 + g224;

                    // compute T
                    A.invert();
                    F += B.leftmultiply(C.rightmultiply(A));
                    Dune::FieldMatrix<Scalar, 2 * dim, 2 * dim> T(F);

                    // assemble the global matrix M_ and right hand side f
                    M_[globalIdx1][globalIdx1] += T[0][0] + T[2][0];
                    M_[globalIdx1][globalIdx2] += T[0][1] + T[2][1];
                    M_[globalIdx1][globalIdx3] += T[0][2] + T[2][2];
                    M_[globalIdx1][globalIdx4] += T[0][3] + T[2][3];

                }
                // 'nextisIt' is on the boundary

                else
                {
                    // computation of flux through the first half edge of 'isIt' and the flux
                    // through the second half edge of 'nextisIt'

                    // get common geometry information for the following computation
                    // get the information of the face 'isIt24' between cell2 and cell4 (locally numbered)
                    IntersectionIterator isIt24 = problem_.gridView().template ibegin(*outside);
                    IntersectionIterator innerisItEnd = problem_.gridView().template iend(*outside);
                    for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*outside); innerisIt
                            != innerisItEnd; ++innerisIt)
                    {
                        if (innerisIt->boundary())
                        {
                            for (int i = 0; i < innerisIt->geometry().corners(); ++i)
                            {
                                GlobalPosition innerisItcorner = innerisIt->geometry().corner(i);

                                if (innerisItcorner == corner1234)
                                {
                                    isIt24 = innerisIt;
                                    continue;
                                }
                            }
                        }
                    }

                    // get geometry type of face 'isIt24'
                    Dune::GeometryType gtf24 = isIt24->geometryInInside().type();

                    // center of face in global coordinates, i.e., the midpoint of edge 'isIt24'
                    GlobalPosition globalPosFace24 = isIt24->geometry().center();

                    // get face volume
                    Scalar face24vol = isIt24->geometry().volume();

                    // get outer normal vector scaled with half volume of face 'isIt24'
                    Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln4 = isIt24->centerUnitOuterNormal();
                    integrationOuterNormaln4 *= face24vol / 2.0;

                    // get boundary condition for boundary face (nextisIt) center
                    BoundaryConditions::Flags nextisItbctype = problem_.bctypePress(globalPosFace13, *nextisIt);

                    // 'nextisIt': Neumann boundary
                    if (nextisItbctype == BoundaryConditions::neumann)
                    {
                        // get Neumann boundary value of 'nextisIt'
                        std::vector<Scalar> J(problem_.neumannPress(globalPosFace13, *nextisIt));
                        Scalar J3 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                        // get boundary condition for boundary face (isIt24) center
                        BoundaryConditions::Flags isIt24bctype = problem_.bctypePress(globalPosFace24, *isIt24);

                        // 'isIt24': Neumann boundary
                        if (isIt24bctype == BoundaryConditions::neumann)
                        {
                            // get neumann boundary value of 'isIt24'
                            std::vector<Scalar> J(problem_.neumannPress(globalPosFace24, *isIt24));
                            Scalar J4 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                            // compute normal vectors nu11,nu21; nu12, nu22;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu12(0);
                            R.umv(globalPosFace24 - globalPos2, nu12);

                            FieldVector nu22(0);
                            R.umv(globalPosFace12 - globalPos2, nu22);

                            // compute dF1, dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu22(0);
                            R.umv(nu22, Rnu22);
                            Scalar dF2 = fabs(nu12 * Rnu22);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K2nu12(0);
                            K2.umv(nu12, K2nu12);
                            FieldVector K2nu22(0);
                            K2.umv(nu22, K2nu22);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g112 = lambda2 * (integrationOuterNormaln1 * K2nu12) / dF2;
                            Scalar g122 = lambda2 * (integrationOuterNormaln1 * K2nu22) / dF2;
                            Scalar g212 = lambda2 * (integrationOuterNormaln4 * K2nu12) / dF2;
                            Scalar g222 = lambda2 * (integrationOuterNormaln4 * K2nu22) / dF2;

                            // compute the matrix T & vector r in v = A^{-1}(Bu + r1) = Tu + r
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, 2 * dim - 1> A(0);
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, dim> B(0);
                            Dune::FieldVector<Scalar, 2 * dim - 1> r1(0), r(0);

                            // evaluate matrix A, B
                            A[0][0] = g111 + g112;
                            A[0][1] = g121;
                            A[0][2] = -g122;
                            A[1][0] = g211;
                            A[1][1] = g221;
                            A[2][0] = -g212;
                            A[2][2] = g222;

                            B[0][0] = g111 + g121;
                            B[0][1] = g112 - g122;
                            B[1][0] = g211 + g221;
                            B[2][1] = g222 - g212;

                            // evaluate vector r1
                            r1[1] = -J3 * nextisIt->geometry().volume() / 2.0;
                            r1[2] = -J4 * isIt24->geometry().volume() / 2.0;

                            // compute T and r
                            A.invert();
                            B.leftmultiply(A);
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, dim> T(B);
                            A.umv(r1, r);

                            // assemble the global matrix M_ and right hand side f
                            M_[globalIdx1][globalIdx1] += g111 + g121 - g111 * T[0][0] - g121 * T[1][0];
                            M_[globalIdx1][globalIdx2] += -g111 * T[0][1] - g121 * T[1][1];
                            f_[globalIdx1] += g111 * r[0] + g121 * r[1];

                        }
                        // 'isIt24': Dirichlet boundary

                        else
                        {
                            // get Dirichlet boundary value on 'isIt24'
                            Scalar g4 = problem_.dirichletPress(globalPosFace24, *isIt24);

                            // compute normal vectors nu11,nu21; nu12, nu22;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu12(0);
                            R.umv(globalPosFace24 - globalPos2, nu12);

                            FieldVector nu22(0);
                            R.umv(globalPosFace12 - globalPos2, nu22);

                            // compute dF1, dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu22(0);
                            R.umv(nu22, Rnu22);
                            Scalar dF2 = fabs(nu12 * Rnu22);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K2nu12(0);
                            K2.umv(nu12, K2nu12);
                            FieldVector K2nu22(0);
                            K2.umv(nu22, K2nu22);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g112 = lambda2 * (integrationOuterNormaln1 * K2nu12) / dF2;
                            Scalar g122 = lambda2 * (integrationOuterNormaln1 * K2nu22) / dF2;

                            // compute the matrix T & vector r in v = A^{-1}(Bu + r1) = Tu + r
                            FieldMatrix A(0), B(0);
                            FieldVector r1(0), r(0);

                            // evaluate matrix A, B
                            A[0][0] = g111 + g112;
                            A[0][1] = g121;
                            A[1][0] = g211;
                            A[1][1] = g221;

                            B[0][0] = g111 + g121;
                            B[0][1] = g112 - g122;
                            B[1][0] = g211 + g221;

                            // evaluate vector r1
                            r1[0] = g122 * g4;
                            r1[1] = -J3 * nextisIt->geometry().volume() / 2.0;

                            // compute T and r
                            A.invert();
                            B.leftmultiply(A);
                            FieldMatrix T(B);
                            A.umv(r1, r);

                            // assemble the global matrix M_ and right hand side f
                            M_[globalIdx1][globalIdx1] += g111 + g121 - g111 * T[0][0] - g121 * T[1][0];
                            M_[globalIdx1][globalIdx2] += -g111 * T[0][1] - g121 * T[1][1];
                            f_[globalIdx1] += g111 * r[0] + g121 * r[1];

                        }
                    }
                    // 'nextisIt': Dirichlet boundary

                    else
                    {
                        // get Dirichlet boundary value of 'nextisIt'
                        Scalar g3 = problem_.dirichletPress(globalPosFace13, *nextisIt);

                        // get boundary condition for boundary face (isIt24) center
                        BoundaryConditions::Flags isIt24bctype = problem_.bctypePress(globalPosFace24, *isIt24);

                        // 'isIt24': Neumann boundary
                        if (isIt24bctype == BoundaryConditions::neumann)
                        {
                            // get Neumann boundary value of 'isIt24'
                            std::vector<Scalar> J(problem_.neumannPress(globalPosFace24, *isIt24));
                            Scalar J4 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                            // compute normal vectors nu11,nu21; nu12, nu22;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu12(0);
                            R.umv(globalPosFace24 - globalPos2, nu12);

                            FieldVector nu22(0);
                            R.umv(globalPosFace12 - globalPos2, nu22);

                            // compute dF1, dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu22(0);
                            R.umv(nu22, Rnu22);
                            Scalar dF2 = fabs(nu12 * Rnu22);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K2nu12(0);
                            K2.umv(nu12, K2nu12);
                            FieldVector K2nu22(0);
                            K2.umv(nu22, K2nu22);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g112 = lambda2 * (integrationOuterNormaln1 * K2nu12) / dF2;
                            Scalar g122 = lambda2 * (integrationOuterNormaln1 * K2nu22) / dF2;
                            Scalar g212 = lambda2 * (integrationOuterNormaln4 * K2nu12) / dF2;
                            Scalar g222 = lambda2 * (integrationOuterNormaln4 * K2nu22) / dF2;

                            // compute the matrix T & vector r in v = A^{-1}(Bu + r1) = Tu + r
                            FieldMatrix A(0), B(0);
                            FieldVector r1(0), r(0);

                            // evaluate matrix A, B
                            A[0][0] = g111 + g112;
                            A[0][1] = -g122;
                            A[1][0] = -g212;
                            A[1][1] = g222;

                            B[0][0] = g111 + g121;
                            B[0][1] = g112 - g122;
                            B[1][1] = g222 - g212;

                            // evaluate vector r1
                            r1[0] = -g121 * g3;
                            r1[1] = -J4 * isIt24->geometry().volume() / 2.0;

                            // compute T and r
                            A.invert();
                            B.leftmultiply(A);
                            FieldMatrix T(B);
                            A.umv(r1, r);

                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += (g111 + g121 - g111 * T[0][0]) + (g211 + g221 - g211
                                    * T[0][0]);
                            M_[globalIdx1][globalIdx2] += -g111 * T[0][1] - g211 * T[0][1];
                            f_[globalIdx1] += (g121 + g221) * g3 + (g111 + g211) * r[0];

                        }
                        // 'isIt24': Dirichlet boundary

                        else
                        {
                            // get Dirichlet boundary value on 'isIt24'
                            Scalar g4 = problem_.dirichletPress(globalPosFace24, *isIt24);

                            // compute normal vectors nu11,nu21; nu12, nu22;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu12(0);
                            R.umv(globalPosFace24 - globalPos2, nu12);

                            FieldVector nu22(0);
                            R.umv(globalPosFace12 - globalPos2, nu22);

                            // compute dF1, dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu22(0);
                            R.umv(nu22, Rnu22);
                            Scalar dF2 = fabs(nu12 * Rnu22);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K2nu12(0);
                            K2.umv(nu12, K2nu12);
                            FieldVector K2nu22(0);
                            K2.umv(nu22, K2nu22);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g112 = lambda2 * (integrationOuterNormaln1 * K2nu12) / dF2;
                            Scalar g122 = lambda2 * (integrationOuterNormaln1 * K2nu22) / dF2;

                            // compute the matrix T & vector r
                            FieldMatrix T(0);
                            FieldVector r(0);

                            Scalar coe = g111 + g112;

                            // evaluate matrix T
                            T[0][0] = g112 * (g111 + g121) / coe;
                            T[0][1] = -g111 * (g112 - g122) / coe;
                            T[1][0] = g221 + g211 * (g112 - g121) / coe;
                            T[1][1] = -g211 * (g112 - g122) / coe;

                            // evaluate vector r
                            r[0] = -(g4 * g122 * g111 + g3 * g112 * g121) / coe;
                            r[1] = -g221 * g3 + (g3 * g211 * g121 - g4 * g211 * g122) / coe;
                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T[0][0] + T[1][0];
                            M_[globalIdx1][globalIdx2] += T[0][1] + T[1][1];
                            f_[globalIdx1] -= r[0] + r[1];

                        }
                    }
                }
            }
            // handle boundary face 'isIt'

            else
            {
                // get boundary condition for boundary face center of 'isIt'
                BoundaryConditions::Flags isItbctype = problem_.bctypePress(globalPosFace12, *isIt);

                // 'isIt' is on Neumann boundary
                if (isItbctype == BoundaryConditions::neumann)
                {
                    // get Neumann boundary value
                    std::vector<Scalar> J(problem_.neumannPress(globalPosFace12, *isIt));
                    Scalar J1 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                    // evaluate right hand side
                    f_[globalIdx1] -= face12vol * J1;

                    // 'nextisIt' is on boundary
                    if (nextisIt->boundary())
                    {
                        // get boundary condition for boundary face center of 'nextisIt'
                        BoundaryConditions::Flags nextisItbctype = problem_.bctypePress(globalPosFace13, *nextisIt);

                        if (nextisItbctype == BoundaryConditions::dirichlet)
                        {
                            // get Dirichlet boundary value
                            Scalar g3 = problem_.dirichletPress(globalPosFace13, *nextisIt);

                            // compute normal vectors nu11,nu21;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            // compute dF1, dF2 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;

                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += g221 - g211 * g121 / g111;
                            f_[globalIdx1] -= (g211 * g121 / g111 - g221) * g3 - (g211 * (-J1) * face12vol) / (2.0
                                    * g111);

                        }
                    }
                    // 'nextisIt' is inside

                    else
                    {
                        // neighbor cell 3
                        // access neighbor cell 3
                        ElementPointer nextisItoutside = nextisIt->outside();
                        int globalIdx3 = problem_.variables().index(*nextisItoutside);

                        // neighbor cell 3 geometry type
                        Dune::GeometryType gt3 = nextisItoutside->geometry().type();

                        // get global coordinate of neighbor cell 3 center
                        GlobalPosition globalPos3 = nextisItoutside->geometry().center();

                        // get absolute permeability of neighbor cell 3
                        FieldMatrix
                                K3(problem_.spatialParameters().intrinsicPermeability(globalPos3, *nextisItoutside));

                        // get the information of the face 'isIt34' between cell3 and cell4 (locally numbered)
                        IntersectionIterator isIt34 = problem_.gridView().template ibegin(*nextisItoutside);
                        IntersectionIterator innernextisItEnd = problem_.gridView().template iend(*nextisItoutside);
                        for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*nextisItoutside); innerisIt
                                != innernextisItEnd; ++innerisIt)
                        {
                            if (innerisIt->boundary())
                            {
                                for (int i = 0; i < innerisIt->geometry().corners(); ++i)
                                {
                                    GlobalPosition innerisItcorner = innerisIt->geometry().corner(i);

                                    if (innerisItcorner == corner1234)
                                    {
                                        isIt34 = innerisIt;
                                        continue;
                                    }
                                }
                            }
                        }

                        // get geometry type of face 'isIt34'
                        Dune::GeometryType gtf34 = isIt34->geometryInInside().type();

                        // center of face in global coordinates, i.e., the midpoint of edge 'isIt34'
                        GlobalPosition globalPosFace34 = isIt34->geometry().center();

                        // get face volume
                        Scalar face34vol = isIt34->geometry().volume();

                        // get outer normal vector scaled with half volume of face 'isIt34'
                        Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln2 = isIt34->centerUnitOuterNormal();
                        integrationOuterNormaln2 *= face34vol / 2.0;

                        // get boundary condition for boundary face center of 'isIt34'
                        BoundaryConditions::Flags isIt34bctype = problem_.bctypePress(globalPosFace34, *isIt34);

                        // 'isIt34': Neumann boundary
                        if (isIt34bctype == BoundaryConditions::neumann)
                        {
                            // get Neumann boundary value
                            std::vector<Scalar> J(problem_.neumannPress(globalPosFace34, *isIt34));
                            Scalar J2 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                            // compute normal vectors nu11,nu21; nu13, nu23;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu13(0);
                            R.umv(globalPos3 - globalPosFace13, nu13);

                            FieldVector nu23(0);
                            R.umv(globalPos3 - globalPosFace34, nu23);

                            // compute dF1, dF3 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu23(0);
                            R.umv(nu23, Rnu23);
                            Scalar dF3 = fabs(nu13 * Rnu23);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K3nu13(0);
                            K3.umv(nu13, K3nu13);
                            FieldVector K3nu23(0);
                            K3.umv(nu23, K3nu23);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g113 = lambda3 * (integrationOuterNormaln2 * K3nu13) / dF3;
                            Scalar g123 = lambda3 * (integrationOuterNormaln2 * K3nu23) / dF3;
                            Scalar g213 = lambda3 * (integrationOuterNormaln3 * K3nu13) / dF3;
                            Scalar g223 = lambda3 * (integrationOuterNormaln3 * K3nu23) / dF3;

                            // compute transmissibility matrix T = CA^{-1}B+F
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, 2 * dim - 1> C(0), A(0);
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, dim> F(0), B(0);

                            // evaluate matrix C, F, A, B
                            C[0][0] = -g111;
                            C[0][2] = -g121;
                            C[1][1] = -g113;
                            C[1][2] = g123;
                            C[2][1] = -g213;
                            C[2][2] = g223;

                            F[0][0] = g111 + g121;
                            F[1][1] = g113 - g123;
                            F[2][1] = g213 - g223;

                            A[0][0] = g111;
                            A[0][2] = g121;
                            A[1][1] = g113;
                            A[1][2] = -g123;
                            A[2][0] = g211;
                            A[2][1] = -g213;
                            A[2][2] = g223 + g221;

                            B[0][0] = g111 + g121;
                            B[1][1] = g113 - g123;
                            B[2][0] = g211 + g221;
                            B[2][1] = g223 - g213;

                            // compute T
                            A.invert();
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, 2 * dim - 1> CAinv(C.rightmultiply(A));
                            F += B.leftmultiply(CAinv);
                            Dune::FieldMatrix<Scalar, 2 * dim - 1, dim> T(F);

                            // compute vector r
                            // evaluate r1
                            Dune::FieldVector<Scalar, 2 * dim - 1> r1(0);
                            r1[0] = -J1 * face12vol / 2.0;
                            r1[1] = -J2 * isIt34->geometry().volume() / 2.0;

                            // compute  r = CA^{-1}r1
                            Dune::FieldVector<Scalar, 2 * dim - 1> r(0);
                            CAinv.umv(r1, r);

                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T[2][0];
                            M_[globalIdx1][globalIdx3] += T[2][1];
                            f_[globalIdx1] -= r[2];

                        }
                        // 'isIt34': Dirichlet boundary

                        else
                        {
                            // get Dirichlet boundary value
                            Scalar g2 = problem_.dirichletPress(globalPosFace34, *isIt34);

                            // compute normal vectors nu11,nu21; nu13, nu23;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu13(0);
                            R.umv(globalPos3 - globalPosFace13, nu13);

                            FieldVector nu23(0);
                            R.umv(globalPos3 - globalPosFace34, nu23);

                            // compute dF1, dF3 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu23(0);
                            R.umv(nu23, Rnu23);
                            Scalar dF3 = fabs(nu13 * Rnu23);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K3nu13(0);
                            K3.umv(nu13, K3nu13);
                            FieldVector K3nu23(0);
                            K3.umv(nu23, K3nu23);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g213 = lambda3 * (integrationOuterNormaln3 * K3nu13) / dF3;
                            Scalar g223 = lambda3 * (integrationOuterNormaln3 * K3nu23) / dF3;

                            // compute transmissibility matrix T = CA^{-1}B+F
                            FieldMatrix C(0), A(0), F(0), B(0);

                            // evaluate matrix C, F, A, B
                            C[0][0] = -g111;
                            C[0][1] = -g121;
                            C[1][1] = g223;

                            F[0][0] = g111 + g121;
                            F[1][1] = g213 - g223;

                            A[0][0] = g111;
                            A[0][1] = g121;
                            A[1][0] = g211;
                            A[1][1] = g223 + g221;

                            B[0][0] = g111 + g121;
                            B[1][0] = g211 + g221;
                            B[1][1] = g223 - g213;

                            // compute T
                            A.invert();
                            FieldMatrix CAinv(C.rightmultiply(A));
                            F += B.leftmultiply(CAinv);
                            FieldMatrix T(F);

                            // compute vector r
                            // evaluate r1, r2
                            FieldVector r1(0), r2(0);
                            r1[1] = -g213 * g2;
                            r2[0] = -J1 * face12vol / 2.0;
                            r2[1] = g213 * g2;

                            // compute  r = CA^{-1}r1
                            FieldVector r(0);
                            CAinv.umv(r2, r);
                            r += r1;

                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T[1][0];
                            M_[globalIdx1][globalIdx3] += T[1][1];
                            f_[globalIdx1] -= r[1];

                        }
                    }
                }
                // 'isIt' is on Dirichlet boundary

                else
                {
                    // get Dirichlet boundary value
                    Scalar g1 = problem_.dirichletPress(globalPosFace12, *isIt);

                    // 'nextisIt' is on boundary
                    if (nextisIt->boundary())
                    {
                        // get boundary condition for boundary face (nextisIt) center
                        BoundaryConditions::Flags nextisItbctype = problem_.bctypePress(globalPosFace13, *nextisIt);

                        // 'nextisIt': Dirichlet boundary
                        if (nextisItbctype == BoundaryConditions::dirichlet)
                        {
                            // get Dirichlet boundary value of 'nextisIt'
                            Scalar g3 = problem_.dirichletPress(globalPosFace13, *nextisIt);

                            // compute normal vectors nu11,nu21;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            // compute dF1 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;

                            // evaluate T1, T3, r1, r3
                            Scalar T1 = g111 + g121;
                            Scalar T3 = g211 + g221;
                            Scalar r1 = g111 * g1 + g121 * g3;
                            Scalar r3 = g211 * g1 + g221 * g3;

                            // assemble matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T1 + T3;
                            f_[globalIdx1] += r1 + r3;
                        }
                        // 'nextisIt': Neumann boundary

                        else
                        {
                            // get Neumann boundary value of 'nextisIt'
                            std::vector<Scalar> J(problem_.neumannPress(globalPosFace13, *nextisIt));
                            Scalar J3 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                            // compute normal vectors nu11,nu21;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            // compute dF1 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;

                            // evaluate T, r
                            Scalar T = g111 - g211 * g121 / g221;
                            Scalar r = -T * g1 - g121 * (-J3) * nextisIt->geometry().volume() / (2.0 * g221);

                            // assemble matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T;
                            f_[globalIdx1] -= r;
                        }
                    }
                    // 'nextisIt' is inside

                    else
                    {
                        // neighbor cell 3
                        // access neighbor cell 3
                        ElementPointer nextisItoutside = nextisIt->outside();
                        int globalIdx3 = problem_.variables().index(*nextisItoutside);

                        // neighbor cell 3 geometry type
                        Dune::GeometryType gt3 = nextisItoutside->geometry().type();

                        // get global coordinate of neighbor cell 3 center
                        GlobalPosition globalPos3 = nextisItoutside->geometry().center();

                        // get absolute permeability of neighbor cell 3
                        FieldMatrix
                                K3(problem_.spatialParameters().intrinsicPermeability(globalPos3, *nextisItoutside));

                        // get the information of the face 'isIt34' between cell3 and cell4 (locally numbered)
                        IntersectionIterator isIt34 = problem_.gridView().template ibegin(*nextisItoutside);
                        IntersectionIterator innernextisItEnd = problem_.gridView().template iend(*nextisItoutside);
                        for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*nextisItoutside); innerisIt
                                != innernextisItEnd; ++innerisIt)
                        {
                            if (innerisIt->boundary())
                            {
                                for (int i = 0; i < innerisIt->geometry().corners(); ++i)
                                {
                                    GlobalPosition innerisItcorner = innerisIt->geometry().corner(i);

                                    if (innerisItcorner == corner1234)
                                    {
                                        isIt34 = innerisIt;
                                        continue;
                                    }
                                }
                            }
                        }

                        // get geometry type of face 'isIt34'
                        Dune::GeometryType gtf34 = isIt34->geometryInInside().type();

                        // center of face in global coordinates, i.e., the midpoint of edge 'isIt34'
                        GlobalPosition globalPosFace34 = isIt34->geometry().center();

                        // get face volume
                        Scalar face34vol = isIt34->geometry().volume();

                        // get outer normal vector scaled with half volume of face 'isIt34'
                        Dune::FieldVector<Scalar, dimWorld> integrationOuterNormaln2 = isIt34->centerUnitOuterNormal();
                        integrationOuterNormaln2 *= face34vol / 2.0;

                        // get boundary condition for boundary face (isIt34) center
                        BoundaryConditions::Flags isIt34bctype = problem_.bctypePress(globalPosFace34, *isIt34);

                        // 'isIt34': Dirichlet boundary
                        if (isIt34bctype == BoundaryConditions::dirichlet)
                        {
                            // get Dirichlet boundary value of 'isIt34'
                            Scalar g2 = problem_.dirichletPress(globalPosFace34, *isIt34);

                            // compute normal vectors nu11,nu21; nu13, nu23;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu13(0);
                            R.umv(globalPos3 - globalPosFace13, nu13);

                            FieldVector nu23(0);
                            R.umv(globalPos3 - globalPosFace34, nu23);

                            // compute dF1, dF3 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu23(0);
                            R.umv(nu23, Rnu23);
                            Scalar dF3 = fabs(nu13 * Rnu23);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K3nu13(0);
                            K3.umv(nu13, K3nu13);
                            FieldVector K3nu23(0);
                            K3.umv(nu23, K3nu23);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g213 = lambda3 * (integrationOuterNormaln3 * K3nu13) / dF3;
                            Scalar g223 = lambda3 * (integrationOuterNormaln3 * K3nu23) / dF3;

                            // compute the matrix T & vector r
                            FieldMatrix T(0);
                            FieldVector r(0);

                            Scalar coe = g221 + g223;

                            // evaluate matrix T
                            T[0][0] = g111 + g121 * (g223 - g211) / coe;
                            T[0][1] = -g121 * (g223 - g213) / coe;
                            T[1][0] = g223 * (g211 + g221) / coe;
                            T[1][1] = -g221 * (g223 - g213) / coe;

                            // evaluate vector r
                            r[0] = -g111 * g1 + (g1 * g121 * g211 - g2 * g213 * g121) / coe;
                            r[1] = -(g1 * g211 * g223 + g2 * g221 * g213) / coe;
                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += T[0][0] + T[1][0];
                            M_[globalIdx1][globalIdx3] += T[0][1] + T[1][1];
                            f_[globalIdx1] -= r[0] + r[1];

                        }
                        // 'isIt34': Neumann boundary

                        else
                        {
                            // get Neumann boundary value of 'isIt34'
                            std::vector<Scalar> J(problem_.neumannPress(globalPosFace34, *isIt34));
                            Scalar J2 = (J[wPhaseIdx] / densityW + J[nPhaseIdx] / densityNW);

                            // compute normal vectors nu11,nu21; nu13, nu23;
                            FieldVector nu11(0);
                            R.umv(globalPosFace13 - globalPos1, nu11);

                            FieldVector nu21(0);
                            R.umv(globalPos1 - globalPosFace12, nu21);

                            FieldVector nu13(0);
                            R.umv(globalPos3 - globalPosFace13, nu13);

                            FieldVector nu23(0);
                            R.umv(globalPos3 - globalPosFace34, nu23);

                            // compute dF1, dF3 i.e., the area of quadrilateral made by normal vectors 'nu'
                            FieldVector Rnu21(0);
                            R.umv(nu21, Rnu21);
                            Scalar dF1 = fabs(nu11 * Rnu21);

                            FieldVector Rnu23(0);
                            R.umv(nu23, Rnu23);
                            Scalar dF3 = fabs(nu13 * Rnu23);

                            // compute components needed for flux calculation, denoted as 'g'
                            FieldVector K1nu11(0);
                            K1.umv(nu11, K1nu11);
                            FieldVector K1nu21(0);
                            K1.umv(nu21, K1nu21);
                            FieldVector K3nu13(0);
                            K3.umv(nu13, K3nu13);
                            FieldVector K3nu23(0);
                            K3.umv(nu23, K3nu23);
                            Scalar g111 = lambda1 * (integrationOuterNormaln1 * K1nu11) / dF1;
                            Scalar g121 = lambda1 * (integrationOuterNormaln1 * K1nu21) / dF1;
                            Scalar g211 = lambda1 * (integrationOuterNormaln3 * K1nu11) / dF1;
                            Scalar g221 = lambda1 * (integrationOuterNormaln3 * K1nu21) / dF1;
                            Scalar g113 = lambda3 * (integrationOuterNormaln2 * K3nu13) / dF3;
                            Scalar g123 = lambda3 * (integrationOuterNormaln2 * K3nu23) / dF3;
                            Scalar g213 = lambda3 * (integrationOuterNormaln3 * K3nu13) / dF3;
                            double g223 = lambda3 * (integrationOuterNormaln3 * K3nu23) / dF3;

                            // compute the matrix T & vector r in v = A^{-1}(Bu + r1) = Tu + r
                            FieldMatrix A(0), B(0);
                            FieldVector r1(0), r(0);

                            // evaluate matrix A, B
                            A[0][0] = g113;
                            A[0][1] = -g123;
                            A[1][0] = -g213;
                            A[1][1] = g221 + g223;

                            B[0][1] = g113 - g123;
                            B[1][0] = g211 + g221;
                            B[1][1] = g223 - g213;

                            // evaluate vector r1
                            r1[0] = -J2 * isIt34->geometry().volume() / 2.0;
                            r1[1] = -g211 * g1;

                            // compute T and r
                            A.invert();
                            B.leftmultiply(A);
                            FieldMatrix T(B);
                            A.umv(r1, r);

                            // assemble the global matrix M_ and right hand side f_
                            M_[globalIdx1][globalIdx1] += (g111 + g121 - g121 * T[1][0]) + (g211 + g221 - g221
                                    * T[1][0]);
                            M_[globalIdx1][globalIdx3] += -g121 * T[1][1] - g221 * T[1][1];
                            f_[globalIdx1] += (g111 + g211) * g1 + (g121 + g221) * r[1];

                        }
                    }
                }
            }

        } // end all intersections

    } // end grid traversal

    // get the number of nonzero terms in the matrix
    Scalar num_nonzero = 0;

    // determine position of matrix entries
    for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != problem_.gridView().template end<0> (); ++eIt)
    {
        // cell index
        int globalIdxI = problem_.variables().index(*eIt);

        if (M_[globalIdxI][globalIdxI] != 0)
            ++num_nonzero;

        // run through all intersections with neighbors
        IntersectionIterator isItBegin = problem_.gridView().template ibegin(*eIt);
        IntersectionIterator isItEnd = problem_.gridView().template iend(*eIt);
        for (IntersectionIterator isIt = isItBegin; isIt != isItEnd; ++isIt)
        {
            IntersectionIterator tempisIt = isIt;
            IntersectionIterator tempisItBegin = isItBegin;

            // 'nextisIt' iterates over next codimension 1 intersection neighboring with 'isIt'
            // sequence of next is anticlockwise of 'isIt'
            IntersectionIterator nextisIt = ++tempisIt;

            // get 'nextisIt'
            switch (GET_PROP_VALUE(TypeTag, PTAG(GridImplementation)))
            {
            // for SGrid
            case GridTypeIndices::sGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
            // for YaspGrid
            case GridTypeIndices::yaspGrid:
            {
                if (nextisIt == isItEnd)
                {
                    nextisIt = isItBegin;
                }
                else
                {
                    nextisIt = ++tempisIt;

                    if (nextisIt == isItEnd)
                    {
                        nextisIt = ++tempisItBegin;
                    }
                }

                break;
            }
                // for UGGrid
            case GridTypeIndices::ugGrid:
            {
                if (nextisIt == isItEnd)
                    nextisIt = isItBegin;

                break;
            }
            default:
            {
                DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
            }
            }

            if (isIt->neighbor())
            {
                // access neighbor
                ElementPointer outside = isIt->outside();
                int globalIdxJ = problem_.variables().index(*outside);

                if (M_[globalIdxI][globalIdxJ] != 0)
                    ++num_nonzero;
            }

            if (isIt->neighbor() && nextisIt->neighbor())
            {
                // access the common neighbor of isIt's and nextisIt's outside
                ElementPointer outside = isIt->outside();
                ElementPointer nextisItoutside = nextisIt->outside();

                IntersectionIterator innerisItEnd = problem_.gridView().template iend(*outside);
                IntersectionIterator innernextisItEnd = problem_.gridView().template iend(*nextisItoutside);

                for (IntersectionIterator innerisIt = problem_.gridView().template ibegin(*outside); innerisIt
                        != innerisItEnd; ++innerisIt)
                    for (IntersectionIterator innernextisIt = problem_.gridView().template ibegin(*nextisItoutside); innernextisIt
                            != innernextisItEnd; ++innernextisIt)
                    {
                        if (innerisIt->neighbor() && innernextisIt->neighbor())
                        {
                            ElementPointer innerisItoutside = innerisIt->outside();
                            ElementPointer innernextisItoutside = innernextisIt->outside();

                            if (innerisItoutside == innernextisItoutside && innerisItoutside != isIt->inside())
                            {
                                int globalIdxJ = problem_.variables().index(*innerisItoutside);

                                if (M_[globalIdxI][globalIdxJ] != 0)
                                    ++num_nonzero;
                            }
                        }
                    }
            }
        } // end of 'for' IntersectionIterator
    } // end of 'for' ElementIterator

    std::cout << "number of nonzero terms in the MPFA O-matrix on level " << problem_.gridView().grid().maxLevel()
            << " nnmat: " << num_nonzero << std::endl;

    return;
}

template<class TypeTag>
void FVMPFAOPressure2PUpwind<TypeTag>::solve()
{
    std::cout << "FVMPFAOPressure2PUpwind: solve for pressure" << std::endl;

    Dune::MatrixAdapter<Matrix, Vector, Vector> op(M_); // make linear operator from M_
    Dune::InverseOperatorResult r;

    if (preconditionerName_ == "SeqILU0")
    {
        // preconditioner object
        Dune::SeqILU0<Matrix, Vector, Vector> preconditioner(M_, 1.0);
        if (solverName_ == "CG")
        {
            // an inverse operator
            Dune::CGSolver<Vector> solver(op, preconditioner, 1E-14, 1000, 1);
            solver.apply(problem_.variables().pressure(), f_, r);
        }
        else if (solverName_ == "BiCGSTAB")
        {
            Dune::BiCGSTABSolver<Vector> solver(op, preconditioner, 1E-14, 1000, 1);
            solver.apply(problem_.variables().pressure(), f_, r);
        }
        else
            DUNE_THROW(Dune::NotImplemented, "FVMPFAOPressure2PUpwind :: solve : combination " << preconditionerName_
                    << " and " << solverName_ << ".");
    }
    else if (preconditionerName_ == "SeqPardiso")
    {
        Dune::SeqPardiso<Matrix, Vector, Vector> preconditioner(M_);
        if (solverName_ == "Loop")
        {
            Dune::LoopSolver<Vector> solver(op, preconditioner, 1E-14, 1000, 1);
            solver.apply(problem_.variables().pressure(), f_, r);
        }
        else if (solverName_ == "BiCGSTAB")
        {
            Dune::BiCGSTABSolver<Vector> solver(op, preconditioner, 1E-14, 1000, 1);
            solver.apply(problem_.variables().pressure(), f_, r);
        }
        else
            DUNE_THROW(Dune::NotImplemented, "FVMPFAOPressure2PUpwind :: solve : combination " << preconditionerName_
                    << " and " << solverName_ << ".");
    }
    else
        DUNE_THROW(Dune::NotImplemented, "FVMPFAOPressure2PUpwind :: solve : preconditioner " << preconditionerName_ << ".");

    //                printmatrix(std::cout, M_, "global stiffness matrix", "row", 11, 3);
    //                printvector(std::cout, f_, "right hand side", "row", 200, 1, 3);
    //                    printvector(std::cout, (problem_.variables().pressure()), "pressure", "row", 200, 1, 3);

    return;
}

//constitutive functions are updated once if new saturations are calculated and stored in the variables object
template<class TypeTag>
void FVMPFAOPressure2PUpwind<TypeTag>::updateMaterialLaws(bool first = false)
{
    FluidState fluidState;

    // iterate through leaf grid an evaluate c0 at cell center
    ElementIterator eItEnd = problem_.gridView().template end<0> ();
    for (ElementIterator eIt = problem_.gridView().template begin<0> (); eIt != eItEnd; ++eIt)
    {
        // get geometry type
        Dune::GeometryType gt = eIt->geometry().type();

        // get cell center in reference element
        const LocalPosition &localPos = ReferenceElementContainer::general(gt).position(0, 0);

        // get global coordinate of cell center
        const GlobalPosition& globalPos = eIt->geometry().global(localPos);

        int globalIdx = problem_.variables().index(*eIt);

        Scalar temperature = problem_.temperature(globalPos, *eIt);
        Scalar referencePressure = problem_.referencePressure(globalPos, *eIt);

        //determine phase saturations from primary saturation variable
        Scalar satW = 0;
        switch (saturationType)
        {
        case Sw:
        {
            satW = problem_.variables().saturation()[globalIdx];
            break;
        }
        case Sn:
        {
            satW = 1 - problem_.variables().saturation()[globalIdx];
            break;
        }
        }

        problem_.variables().capillaryPressure(globalIdx) = MaterialLaw::pC(
                problem_.spatialParameters().materialLawParams(globalPos, *eIt), satW);

        Scalar densityW = 0;
        Scalar densityNW = 0;
        Scalar viscosityW = 0;
        Scalar viscosityNW = 0;

        fluidState.update(satW, referencePressure, referencePressure, temperature);

        densityW = FluidSystem::phaseDensity(wPhaseIdx, temperature, referencePressure, fluidState);
        densityNW = FluidSystem::phaseDensity(nPhaseIdx, temperature, referencePressure, fluidState);

        viscosityW = FluidSystem::phaseViscosity(wPhaseIdx, temperature, referencePressure, fluidState);
        viscosityNW = FluidSystem::phaseViscosity(nPhaseIdx, temperature, referencePressure, fluidState);

        Scalar relPermW = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos, *eIt), satW);
        Scalar relPermNW = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos, *eIt), satW);

        Scalar mobilityW = relPermW / viscosityW;
        Scalar mobilityNW = relPermNW / viscosityNW;

        // initialize mobilities
        problem_.variables().mobilityWetting(globalIdx) = mobilityW;
        problem_.variables().mobilityNonwetting(globalIdx) = mobilityNW;

        if (first)
        {
            for (int i = 0; i < 2 * dim; i++)
            {
                for (int j = 0; j < 2 * dim; j++)
                {
                    problem_.variables().upwindMobilitiesWetting(globalIdx, i, j) = mobilityW;
                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, i, j) = mobilityNW;
                }
            }
        }
        else
        {
            IntersectionIterator isItBegin = problem_.gridView().template ibegin(*eIt);
            IntersectionIterator isItEnd = problem_.gridView().template iend(*eIt);
            for (IntersectionIterator isIt = problem_.gridView().template ibegin(*eIt); isIt != isItEnd; ++isIt)
            {
                // intersection iterator 'inextIsIt' is used to get geometry information
                IntersectionIterator tempIsIt = isIt;
                IntersectionIterator tempIsItBegin = isItBegin;

                IntersectionIterator nextIsIt = ++tempIsIt;

                //get nextIsIt
                switch (GET_PROP_VALUE(TypeTag, PTAG(GridImplementation)))
                {
                // for SGrid
                case GridTypeIndices::sGrid:
                {
                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = isItBegin;
                    }
                    else
                    {
                        nextIsIt = ++tempIsIt;

                        if (nextIsIt == isItEnd)
                        {
                            nextIsIt = ++tempIsItBegin;
                        }
                    }

                    break;
                }
                // for YaspGrid
                case GridTypeIndices::yaspGrid:
                {
                    if (nextIsIt == isItEnd)
                    {
                        nextIsIt = isItBegin;
                    }
                    else
                    {
                        nextIsIt = ++tempIsIt;

                        if (nextIsIt == isItEnd)
                        {
                            nextIsIt = ++tempIsItBegin;
                        }
                    }

                    break;
                }
                    // for UGGrid
                case GridTypeIndices::ugGrid:
                {
                    if (nextIsIt == isItEnd)
                        nextIsIt = isItBegin;

                    break;
                }
                default:
                {
                    DUNE_THROW(Dune::NotImplemented, "GridType can not be used with MPFAO implementation!");
                }
                }

                int indexInInside = isIt->indexInInside();
                int nextIndexInInside = nextIsIt->indexInInside();

                // get the intersection node /bar^{x_3} between 'isIt' and 'nextIsIt', denoted as 'corner1234'
                // initialization of corner1234
                GlobalPosition corner1234(0);

                // get the global coordinate of corner1234
                for (int i = 0; i < isIt->geometry().corners(); ++i)
                {
                    const GlobalPosition& isItCorner = isIt->geometry().corner(i);

                    for (int j = 0; j < nextIsIt->geometry().corners(); ++j)
                    {
                        const GlobalPosition& nextIsItCorner = nextIsIt->geometry().corner(j);

                        if (nextIsItCorner == isItCorner)
                        {
                            corner1234 = isItCorner;
                            continue;
                        }
                    }
                }

                if (isIt->neighbor())
                {
                    ElementPointer cellTwo = isIt->outside();
                    int globalIdx2 = problem_.variables().index(*cellTwo);

                    const GlobalPosition& globalPos2 = cellTwo->geometry().center();

                    if (nextIsIt->neighbor())
                    {
                        ElementPointer cellThree = nextIsIt->outside();
                        int globalIdx3 = problem_.variables().index(*cellThree);

                        // get global coordinate of neighbor cell 3 center
                        const GlobalPosition& globalPos3 = cellThree->geometry().center();

                        IntersectionIterator isItTwoEnd = problem_.gridView().template iend(*cellTwo);
                        IntersectionIterator isItThreeEnd = problem_.gridView().template iend(*cellThree);
                        for (IntersectionIterator isItTwo = problem_.gridView().template ibegin(*cellTwo); isItTwo
                                != isItTwoEnd; ++isItTwo)
                        {
                            for (IntersectionIterator isItThree = problem_.gridView().template ibegin(*cellThree); isItThree
                                    != isItThreeEnd; ++isItThree)
                            {
                                if (isItTwo->neighbor() && isItThree->neighbor())
                                {
                                    ElementPointer cellTwoNeighbor = isItTwo->outside();
                                    ElementPointer cellThreeNeighbor = isItThree->outside();

                                    // find the common neighbor cell between cell 2 and cell 3, except cell 1
                                    if (cellTwoNeighbor == cellThreeNeighbor && cellTwoNeighbor != *eIt)
                                    {
                                        ElementPointer cellFour = isItTwo->outside();

                                        int indexIn42 = isItTwo->indexInOutside();
                                        int indexIn43 = isItThree->indexInOutside();

                                        // access neighbor cell 4
                                        int globalIdx4 = problem_.variables().index(*cellTwoNeighbor);

                                        // get global coordinate of neighbor cell 4 center
                                        const GlobalPosition& globalPos4 = cellTwoNeighbor->geometry().center();

                                        Scalar satUpw1 = 0;
                                        Scalar satUpw2 = 0;
                                        Scalar satUpw3 = 0;
                                        Scalar satUpw4 = 0;

                                        Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                        Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                        Scalar faceArea14 = 0.5 * isItTwo->geometry().volume();
                                        Scalar faceArea24 = 0.5 * isItThree->geometry().volume();

                                        FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                        FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                        FieldVector unitOuterNormal12(isItTwo->centerUnitOuterNormal());
                                        FieldVector unitOuterNormal22(isIt->centerUnitOuterNormal());
                                        unitOuterNormal22 *= -1;
                                        FieldVector unitOuterNormal13(nextIsIt->centerUnitOuterNormal());
                                        unitOuterNormal13 *= -1;
                                        FieldVector unitOuterNormal23(isItThree->centerUnitOuterNormal());
                                        FieldVector unitOuterNormal14(isItTwo->centerUnitOuterNormal());
                                        unitOuterNormal14 *= -1;
                                        FieldVector unitOuterNormal24(isItThree->centerUnitOuterNormal());
                                        unitOuterNormal24 *= -1;

                                        FieldVector velocity12(0);
                                        FieldVector velocity13(0);
                                        FieldVector velocity42(0);
                                        FieldVector velocity43(0);

                                        switch (velocityType_)
                                        {
                                        case vw:
                                        {
                                            velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                        +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                            velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                        +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                            velocity42 = problem_.variables().velocity()[globalIdx4][indexIn42]
                                                        + problem_.variables().velocitySecondPhase()[globalIdx4][indexIn42];
                                            velocity43 = problem_.variables().velocity()[globalIdx4][indexIn43]
                                                         + problem_.variables().velocitySecondPhase()[globalIdx4][indexIn43];

                                            break;
                                        }
                                        case vn:
                                        {
                                            velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                        +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                            velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                        +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                            velocity42 = problem_.variables().velocity()[globalIdx4][indexIn42]
                                                        + problem_.variables().velocitySecondPhase()[globalIdx4][indexIn42];
                                            velocity43 = problem_.variables().velocity()[globalIdx4][indexIn43]
                                                         + problem_.variables().velocitySecondPhase()[globalIdx4][indexIn43];

                                            break;
                                        }
                                        case vt:
                                        {
                                            velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                            velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                            velocity42 = problem_.variables().velocity()[globalIdx4][indexIn42];
                                            velocity43 = problem_.variables().velocity()[globalIdx4][indexIn43];

                                            break;
                                        }
                                        }

                                        FieldVector velocityInteractionVol(0);
                                        if (unitOuterNormal21[0] != 0)
                                        {
                                            velocityInteractionVol[0] += faceArea21 * velocity12[0]
                                                    + faceArea24 * velocity43[0];
                                            velocityInteractionVol[1] += faceArea11 * velocity13[1]
                                                    + faceArea14 * velocity42[1];

                                            velocityInteractionVol[0] /= (faceArea21 + faceArea24);
                                            velocityInteractionVol[1] /= (faceArea11 + faceArea14);
                                        }
                                        else
                                        {
                                            velocityInteractionVol[0] += faceArea11 * velocity13[0]
                                                    + faceArea14 * velocity42[0];
                                            velocityInteractionVol[1] += faceArea21 * velocity12[1]
                                                    + faceArea24* velocity43[1];

                                            velocityInteractionVol[0] /= (faceArea11 + faceArea14);
                                            velocityInteractionVol[1] /= (faceArea21 + faceArea24);
                                        }

                                        Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                        Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                        Scalar potentialDiag1 = velocityInteractionVol * (unitOuterNormal11
                                                + unitOuterNormal21);
                                        Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                        Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                        if (std::abs(maxPot) >= std::abs(minPot))
                                        {
                                            satUpw1 = problem_.variables().saturation()[globalIdx];
                                        }
                                        else
                                        {
                                            if (minPot == potentialDiag1)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx4];
                                            }
                                            else if (minPot == potential11)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx3];
                                            }
                                            else if (minPot == potential21)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx2];
                                            }
                                            if (minPot == potentialDiag1 && potentialDiag1 == potential11
                                                    && potentialDiag1 != potential21)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx3];
                                            }
                                            else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                                    && potentialDiag1 != potential11)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx2];
                                            }
                                            else if (minPot == potential21 && potential21 == potential11)
                                            {
                                                satUpw1 = problem_.variables().saturation()[globalIdx4];
                                            }
                                        }
                                        Scalar potential12 = velocityInteractionVol * unitOuterNormal12;
                                        Scalar potential22 = velocityInteractionVol * unitOuterNormal22; //minus sign because of wrong direction of normal vector!
                                        Scalar potentialDiag2 = velocityInteractionVol * (unitOuterNormal12
                                                + unitOuterNormal22);
                                        maxPot = std::max(std::max(potential12, potential22), potentialDiag2);
                                        minPot = std::min(std::min(potential12, potential22), potentialDiag2);

                                        if (std::abs(maxPot) >= std::abs(minPot))
                                        {
                                            satUpw2 = problem_.variables().saturation()[globalIdx2];
                                        }
                                        else
                                        {
                                            if (minPot == potential12)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx4];
                                            }
                                            else if (minPot == potential22)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx];
                                            }
                                            else if (minPot == potentialDiag2)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx3];
                                            }
                                            if (minPot == potentialDiag2 && potentialDiag2 == potential12
                                                    && potentialDiag2 != potential22)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx4];
                                            }
                                            else if (minPot == potentialDiag2 && potentialDiag2 == potential22
                                                    && potentialDiag2 != potential12)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx];
                                            }
                                            else if (minPot == potential22 && potential22 == potential12)
                                            {
                                                satUpw2 = problem_.variables().saturation()[globalIdx3];
                                            }
                                        }
                                        Scalar potential13 = velocityInteractionVol * unitOuterNormal13;
                                        Scalar potential23 = velocityInteractionVol * unitOuterNormal23;
                                        Scalar potentialDiag3 = velocityInteractionVol * (unitOuterNormal13
                                                + unitOuterNormal23);
                                        maxPot = std::max(std::max(potential13, potential23), potentialDiag3);
                                        minPot = std::min(std::min(potential13, potential23), potentialDiag3);

                                        if (std::abs(maxPot) >= std::abs(minPot))
                                        {
                                            satUpw3 = problem_.variables().saturation()[globalIdx3];
                                        }
                                        else
                                        {
                                            if (minPot == potential13)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx];
                                            }
                                            else if (minPot == potential23)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx4];
                                            }
                                            else if (minPot == potentialDiag3)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx2];
                                            }
                                            if (minPot == potentialDiag3 && potentialDiag3 == potential13
                                                    && potentialDiag3 != potential23)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx];
                                            }
                                            else if (minPot == potentialDiag3 && potentialDiag3 == potential23
                                                    && potentialDiag3 != potential13)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx4];
                                            }
                                            else if (minPot == potential23 && potential23 == potential13)
                                            {
                                                satUpw3 = problem_.variables().saturation()[globalIdx2];
                                            }
                                        }

                                        Scalar potential14 = velocityInteractionVol * unitOuterNormal14;
                                        Scalar potential24 = velocityInteractionVol * unitOuterNormal24;
                                        Scalar potentialDiag4 = velocityInteractionVol * (unitOuterNormal14
                                                + unitOuterNormal24);
                                        maxPot = std::max(std::max(potential14, potential24), potentialDiag4);
                                        minPot = std::min(std::min(potential14, potential24), potentialDiag4);

                                        if (std::abs(maxPot) >= std::abs(minPot))
                                        {
                                            satUpw4 = problem_.variables().saturation()[globalIdx4];
                                        }
                                        else
                                        {
                                            if (minPot == potential14)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx2];
                                            }
                                            else if (minPot == potential24)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx3];
                                            }
                                            else if (minPot == potentialDiag4)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx];
                                            }
                                            if (minPot == potentialDiag4 && potentialDiag4 == potential14
                                                    && potentialDiag4 != potential24)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx2];
                                            }
                                            else if (minPot == potentialDiag4 && potentialDiag4 == potential24
                                                    && potentialDiag4 != potential14)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx3];
                                            }
                                            else if (minPot == potential24 && potential24 == potential14)
                                            {
                                                satUpw4 = problem_.variables().saturation()[globalIdx];
                                            }
                                        }
                                        //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw2 = " << satUpw2 << "satUpw3 = "
                                        //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                        problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                                = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                        globalPos, *eIt), satUpw1) / viscosityW;
                                        problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                                = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                        globalPos, *eIt), satUpw1) / viscosityNW;
                                        problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                                = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                        globalPos2, *cellTwo), satUpw2) / viscosityW;
                                        problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                                = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                        globalPos2, *cellTwo), satUpw2) / viscosityNW;
                                        problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                                = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                        globalPos3, *cellThree), satUpw3) / viscosityW;
                                        problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                                = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                        globalPos3, *cellThree), satUpw3) / viscosityNW;
                                        problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 3)
                                                = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                        globalPos4, *cellFour), satUpw4) / viscosityW;
                                        problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 3)
                                                = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                        globalPos4, *cellFour), satUpw4) / viscosityNW;
                                    }
                                }
                            }
                        }
                    }
                    //isItNext is boundary
                    else
                    {
                        // get common geometry information for the following computation
                        // get the information of the face 'isIt24' between cell2 and cell4 (locally numbered)
                        IntersectionIterator isIt24 = problem_.gridView().template ibegin(*cellTwo);
                        IntersectionIterator isItTwoEnd = problem_.gridView().template iend(*cellTwo);
                        for (IntersectionIterator isItTwo = problem_.gridView().template ibegin(*cellTwo); isItTwo
                                != isItTwoEnd; ++isItTwo)
                        {
                            if (isItTwo->boundary())
                            {
                                for (int i = 0; i < isItTwo->geometry().corners(); ++i)
                                {
                                    const GlobalPosition& isItTwoCorner = isItTwo->geometry().corner(i);

                                    if (isItTwoCorner == corner1234)
                                    {
                                        isIt24 = isItTwo;
                                        continue;
                                    }
                                }
                            }
                        }

                        // center of face in global coordinates, i.e., the midpoint of edge 'nextIsIt'
                        const GlobalPosition& globalPosFace13 = nextIsIt->geometry().center();
                        // center of face in global coordinates, i.e., the midpoint of edge 'isIt24'
                        const GlobalPosition& globalPosFace24 = isIt24->geometry().center();

                        // get boundary condition for boundary face (nextIsIt) center
                        BoundaryConditions::Flags nextIsItBCType = problem_.bctypeSat(globalPosFace13, *nextIsIt);
                        // get boundary condition for boundary face (isIt24) center
                        BoundaryConditions::Flags isIt24BCType = problem_.bctypeSat(globalPosFace24, *isIt24);

                        // 'nextIsIt': Dirichlet boundary
                        if (nextIsItBCType == BoundaryConditions::dirichlet)
                        {
                            // 'isIt24': Neumann boundary
                            if (isIt24BCType == BoundaryConditions::neumann)
                            {
                                int indexIn24 = isIt24->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw2 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea12 = 0.5 * isIt24->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal12(isIt24->centerUnitOuterNormal());
                                FieldVector unitOuterNormal22(isIt->centerUnitOuterNormal());
                                unitOuterNormal22 *= -1;


                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity24(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1]
                                            + faceArea12 * velocity24[1];

                                    velocityInteractionVol[0] /= (faceArea21);
                                    velocityInteractionVol[1] /= (faceArea11 + faceArea12);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0]
                                            + faceArea12 * velocity24[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1];

                                    velocityInteractionVol[0] /= (faceArea11 + faceArea12);
                                    velocityInteractionVol[1] /= (faceArea21);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                }
                                Scalar potential12 = velocityInteractionVol * unitOuterNormal12;
                                Scalar potential22 = velocityInteractionVol * unitOuterNormal22; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag2 = velocityInteractionVol
                                        * (unitOuterNormal12 + unitOuterNormal22);
                                maxPot = std::max(std::max(potential12, potential22), potentialDiag2);
                                minPot = std::min(std::min(potential12, potential22), potentialDiag2);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw2 = problem_.variables().saturation()[globalIdx2];
                                }
                                else
                                {
                                    if (minPot == potential12)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potential22)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag2)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    if (minPot == potentialDiag2 && potentialDiag2 == potential12 && potentialDiag2
                                            != potential22)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potentialDiag2 && potentialDiag2 == potential22
                                            && potentialDiag2 != potential12)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential22 && potential22 == potential12)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw2 = " << satUpw2 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityNW;
                            }
                            // 'isIt24': Dirichlet boundary
                            if (isIt24BCType == BoundaryConditions::dirichlet)
                            {
                                int indexIn24 = isIt24->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw2 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea12 = 0.5 * isIt24->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal12(isIt24->centerUnitOuterNormal());
                                FieldVector unitOuterNormal22(isIt->centerUnitOuterNormal());
                                unitOuterNormal22 *= -1;

                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity24(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1]
                                            + faceArea12 * velocity24[1];

                                    velocityInteractionVol[0] /= (faceArea21);
                                    velocityInteractionVol[1] /= (faceArea11 + faceArea12);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0]
                                            + faceArea12 * velocity24[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1];

                                    velocityInteractionVol[0] /= (faceArea11 + faceArea12);
                                    velocityInteractionVol[1] /= (faceArea21);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                }
                                Scalar potential12 = velocityInteractionVol * unitOuterNormal12;
                                Scalar potential22 = velocityInteractionVol * unitOuterNormal22; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag2 = velocityInteractionVol
                                        * (unitOuterNormal12 + unitOuterNormal22);
                                maxPot = std::max(std::max(potential12, potential22), potentialDiag2);
                                minPot = std::min(std::min(potential12, potential22), potentialDiag2);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw2 = problem_.variables().saturation()[globalIdx2];
                                }
                                else
                                {
                                    if (minPot == potential12)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potential22)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag2)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    if (minPot == potentialDiag2 && potentialDiag2 == potential12 && potentialDiag2
                                            != potential22)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potentialDiag2 && potentialDiag2 == potential22
                                            && potentialDiag2 != potential12)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential22 && potential22 == potential12)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw2 = " << satUpw2 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityNW;

                            }
                        }
                        else if (nextIsItBCType == BoundaryConditions::neumann)
                        {
                            // 'isIt24': Dirichlet boundary
                            if (isIt24BCType == BoundaryConditions::dirichlet)
                            {
                                int indexIn24 = isIt24->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw2 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea12 = 0.5 * isIt24->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal12(isIt24->centerUnitOuterNormal());
                                FieldVector unitOuterNormal22(isIt->centerUnitOuterNormal());
                                unitOuterNormal22 *= -1;

                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity24(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24]
                                                +problem_.variables().velocitySecondPhase()[globalIdx2][indexIn24];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity24 = problem_.variables().velocity()[globalIdx2][indexIn24];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1]
                                            + faceArea12 * velocity24[1];

                                    velocityInteractionVol[0] /= (faceArea21);
                                    velocityInteractionVol[1] /= (faceArea11 + faceArea12);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0]
                                            + faceArea12 * velocity24[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1];

                                    velocityInteractionVol[0] /= (faceArea11 + faceArea12);
                                    velocityInteractionVol[1] /= (faceArea21);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx2];
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                }
                                Scalar potential12 = velocityInteractionVol * unitOuterNormal12;
                                Scalar potential22 = velocityInteractionVol * unitOuterNormal22; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag2 = velocityInteractionVol
                                        * (unitOuterNormal12 + unitOuterNormal22);
                                maxPot = std::max(std::max(potential12, potential22), potentialDiag2);
                                minPot = std::min(std::min(potential12, potential22), potentialDiag2);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw2 = problem_.variables().saturation()[globalIdx2];
                                }
                                else
                                {
                                    if (minPot == potential12)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potential22)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag2)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    if (minPot == potentialDiag2 && potentialDiag2 == potential12 && potentialDiag2
                                            != potential22)
                                    {
                                        satUpw2 = problem_.dirichletSat(globalPosFace24, *isIt24);
                                    }
                                    else if (minPot == potentialDiag2 && potentialDiag2 == potential22
                                            && potentialDiag2 != potential12)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential22 && potential22 == potential12)
                                    {
                                        satUpw2 = problem_.variables().saturation()[globalIdx];
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw2 = " << satUpw2 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos2,
                                                *cellTwo), satUpw2) / viscosityNW;
                            }
                            // 'isIt24': neumann boundary
                            if (isIt24BCType == BoundaryConditions::neumann)
                            {
                                //no given saturations at the boundary
                                if (problem_.variables().potentialWetting(globalIdx, indexInInside) >= 0)
                                {
                                    Scalar satUpw = problem_.variables().saturation()[globalIdx];

                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                            = mobilityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                            = mobilityNW;

                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos2, *cellTwo), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos2, *cellTwo), satUpw) / viscosityNW;

                                }
                                else
                                {
                                    Scalar satUpw = problem_.variables().saturation()[globalIdx2];
                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos, *eIt), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos, *eIt), satUpw) / viscosityNW;
                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 1)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos2, *cellTwo), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos2, *cellTwo), satUpw) / viscosityNW;
                                }
//                                std::cout << "upwind mobilities 0  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 0) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
//                                        << "\n";
//
//                                std::cout << "upwind mobilities 1  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 1) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 1)
//                                        << "\n";
                            }
                        }
                    }
                }
                //isIt is boundary
                else
                {
                    // center of face in global coordinates, i.e., the midpoint of edge 'isIt'
                    const GlobalPosition& globalPosFace12 = isIt->geometry().center();

                    // get boundary condition for boundary face (nextIsIt) center
                    BoundaryConditions::Flags isItBCType = problem_.bctypeSat(globalPosFace12, *isIt);

                    if (nextIsIt->neighbor())
                    {
                        ElementPointer cellThree = nextIsIt->outside();

                        int globalIdx3 = problem_.variables().index(*cellThree);

                        const GlobalPosition& globalPos3 = cellThree->geometry().center();

                        // get common geometry information for the following computation
                        // get the information of the face 'isIt34' between cell3 and cell4 (locally numbered)
                        IntersectionIterator isIt34 = problem_.gridView().template ibegin(*cellThree);
                        IntersectionIterator isItThreeEnd = problem_.gridView().template iend(*cellThree);
                        for (IntersectionIterator isItThree = problem_.gridView().template ibegin(*cellThree); isItThree
                                != isItThreeEnd; ++isItThree)
                        {
                            if (isItThree->boundary())
                            {
                                for (int i = 0; i < isItThree->geometry().corners(); ++i)
                                {
                                    const GlobalPosition& isItThreeCorner = isItThree->geometry().corner(i);

                                    if (isItThreeCorner == corner1234)
                                    {
                                        isIt34 = isItThree;
                                        continue;
                                    }
                                }
                            }
                        }

                        // center of face in global coordinates, i.e., the midpoint of edge 'isIt34'
                        const GlobalPosition& globalPosFace34 = isIt34->geometry().center();

                        // get boundary condition for boundary face (isIt34) center
                        BoundaryConditions::Flags isIt34BCType = problem_.bctypeSat(globalPosFace34, *isIt34);

                        // 'isIt': Neumann boundary
                        if (isItBCType == BoundaryConditions::neumann)
                        {
                            // 'isIt34': Neumann boundary
                            if (isIt34BCType == BoundaryConditions::dirichlet)
                            {
                                int indexIn34 = isIt34->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw3 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea23 = 0.5 * isIt34->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal13(nextIsIt->centerUnitOuterNormal());
                                unitOuterNormal13 *= -1;
                                FieldVector unitOuterNormal23(isIt34->centerUnitOuterNormal());

                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity34(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0] + faceArea23
                                            * velocity34[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1];

                                    velocityInteractionVol[0] /= (faceArea21 + faceArea23);
                                    velocityInteractionVol[1] /= (faceArea11);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1] + faceArea23
                                            * velocity34[0];

                                    velocityInteractionVol[0] /= (faceArea11);
                                    velocityInteractionVol[1] /= (faceArea21 + faceArea23);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx];
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                }
                                Scalar potential13 = velocityInteractionVol * unitOuterNormal13;
                                Scalar potential23 = velocityInteractionVol * unitOuterNormal23; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag3 = velocityInteractionVol
                                        * (unitOuterNormal13 + unitOuterNormal23);
                                maxPot = std::max(std::max(potential13, potential23), potentialDiag3);
                                minPot = std::min(std::min(potential13, potential23), potentialDiag3);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw3 = problem_.variables().saturation()[globalIdx3];
                                }
                                else
                                {
                                    if (minPot == potential13)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential23)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potentialDiag3)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    if (minPot == potentialDiag3 && potentialDiag3 == potential13 && potentialDiag3
                                            != potential23)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag3 && potentialDiag3 == potential23
                                            && potentialDiag3 != potential13)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potential23 && potential23 == potential13)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw3 = " << satUpw3 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityNW;

//                                std::cout << "upwind mobilities 0  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 0) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
//                                        << "\n";
//
//                                std::cout << "upwind mobilities 2  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 2) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
//                                        << "\n";

                            }
                            // 'isIt34': Neumann boundary
                            if (isIt34BCType == BoundaryConditions::neumann)
                            {
                                //no given saturations at the boundary
                                if (problem_.variables().potentialWetting(globalIdx, nextIndexInInside) >= 0)
                                {
                                    Scalar satUpw = problem_.variables().saturation()[globalIdx];

                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                            = mobilityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                            = mobilityNW;

                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos3, *cellThree), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos3, *cellThree), satUpw) / viscosityNW;
                                }
                                else
                                {
                                    Scalar satUpw = problem_.variables().saturation()[globalIdx3];
                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos, *eIt), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos, *eIt), satUpw) / viscosityNW;
                                    problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                            = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(
                                                    globalPos3, *cellThree), satUpw) / viscosityW;
                                    problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                            = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(
                                                    globalPos3, *cellThree), satUpw) / viscosityNW;
                                }

//                                std::cout << "upwind mobilities 0  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 0) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
//                                        << "\n";
//
//                                std::cout << "upwind mobilities 2  = " << problem_.variables().upwindMobilitiesWetting(
//                                        globalIdx, indexInInside, 2) << ", "
//                                        << problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
//                                        << "\n";

                            }
                        }
                        // 'isIt': Dirichlet boundary
                        else if (isItBCType == BoundaryConditions::dirichlet)
                        {
                            // 'isIt34': Neumann boundary
                            if (isIt34BCType == BoundaryConditions::neumann)
                            {
                                int indexIn34 = isIt34->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw3 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea23 = 0.5 * isIt34->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal13(nextIsIt->centerUnitOuterNormal());
                                unitOuterNormal13 *= -1;
                                FieldVector unitOuterNormal23(isIt34->centerUnitOuterNormal());

                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity34(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0] + faceArea23
                                            * velocity34[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1];

                                    velocityInteractionVol[0] /= (faceArea21 + faceArea23);
                                    velocityInteractionVol[1] /= (faceArea11);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1] + faceArea23
                                            * velocity34[0];

                                    velocityInteractionVol[0] /= (faceArea11);
                                    velocityInteractionVol[1] /= (faceArea21 + faceArea23);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                }
                                Scalar potential13 = velocityInteractionVol * unitOuterNormal13;
                                Scalar potential23 = velocityInteractionVol * unitOuterNormal23; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag3 = velocityInteractionVol
                                        * (unitOuterNormal13 + unitOuterNormal23);
                                maxPot = std::max(std::max(potential13, potential23), potentialDiag3);
                                minPot = std::min(std::min(potential13, potential23), potentialDiag3);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw3 = problem_.variables().saturation()[globalIdx3];
                                }
                                else
                                {
                                    if (minPot == potential13)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential23)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potentialDiag3)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    if (minPot == potentialDiag3 && potentialDiag3 == potential13 && potentialDiag3
                                            != potential23)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag3 && potentialDiag3 == potential23
                                            && potentialDiag3 != potential13)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potential23 && potential23 == potential13)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw3 = " << satUpw3 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityNW;
                            }
                            // 'isIt34': Dirichlet boundary
                            if (isIt34BCType == BoundaryConditions::dirichlet)
                            {
                                int indexIn34 = isIt34->indexInInside();

                                Scalar satUpw1 = 0;
                                Scalar satUpw3 = 0;

                                Scalar faceArea11 = 0.5 * nextIsIt->geometry().volume();
                                Scalar faceArea21 = 0.5 * isIt->geometry().volume();
                                Scalar faceArea23 = 0.5 * isIt34->geometry().volume();

                                FieldVector unitOuterNormal11(nextIsIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal21(isIt->centerUnitOuterNormal());
                                FieldVector unitOuterNormal13(nextIsIt->centerUnitOuterNormal());
                                unitOuterNormal13 *= -1;
                                FieldVector unitOuterNormal23(isIt34->centerUnitOuterNormal());

                                FieldVector velocity12(0);
                                FieldVector velocity13(0);
                                FieldVector velocity34(0);

                                switch (velocityType_)
                                {
                                case vw:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vn:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside]
                                                +problem_.variables().velocitySecondPhase()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34]
                                                +problem_.variables().velocitySecondPhase()[globalIdx3][indexIn34];
                                    break;
                                }
                                case vt:
                                {
                                    velocity12 = problem_.variables().velocity()[globalIdx][indexInInside];
                                    velocity13 = problem_.variables().velocity()[globalIdx][nextIndexInInside];
                                    velocity34 = problem_.variables().velocity()[globalIdx3][indexIn34];
                                    break;
                                }
                                }

                                FieldVector velocityInteractionVol(0);
                                if (unitOuterNormal21[0] != 0)
                                {
                                    velocityInteractionVol[0] += faceArea21
                                            * velocity12[0] + faceArea23
                                            * velocity34[0];
                                    velocityInteractionVol[1] += faceArea11
                                            * velocity13[1];

                                    velocityInteractionVol[0] /= (faceArea21 + faceArea23);
                                    velocityInteractionVol[1] /= (faceArea11);
                                }
                                else
                                {
                                    velocityInteractionVol[0] += faceArea11
                                            * velocity13[0];
                                    velocityInteractionVol[1] += faceArea21
                                            * velocity12[1] + faceArea23
                                            * velocity34[0];

                                    velocityInteractionVol[0] /= (faceArea11);
                                    velocityInteractionVol[1] /= (faceArea21 + faceArea23);
                                }

                                Scalar potential11 = velocityInteractionVol * unitOuterNormal11;
                                Scalar potential21 = velocityInteractionVol * unitOuterNormal21;
                                Scalar potentialDiag1 = velocityInteractionVol
                                        * (unitOuterNormal11 + unitOuterNormal21);
                                Scalar maxPot = std::max(std::max(potential11, potential21), potentialDiag1);
                                Scalar minPot = std::min(std::min(potential11, potential21), potentialDiag1);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw1 = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potentialDiag1)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potential11)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    if (minPot == potentialDiag1 && potentialDiag1 == potential11 && potentialDiag1
                                            != potential21)
                                    {
                                        satUpw1 = problem_.variables().saturation()[globalIdx3];
                                    }
                                    else if (minPot == potentialDiag1 && potentialDiag1 == potential21
                                            && potentialDiag1 != potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw1 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                }
                                Scalar potential13 = velocityInteractionVol * unitOuterNormal13;
                                Scalar potential23 = velocityInteractionVol * unitOuterNormal23; //minus sign because of wrong direction of normal vector!
                                Scalar potentialDiag3 = velocityInteractionVol
                                        * (unitOuterNormal13 + unitOuterNormal23);
                                maxPot = std::max(std::max(potential13, potential23), potentialDiag3);
                                minPot = std::min(std::min(potential13, potential23), potentialDiag3);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw3 = problem_.variables().saturation()[globalIdx3];
                                }
                                else
                                {
                                    if (minPot == potential13)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential23)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potentialDiag3)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    if (minPot == potentialDiag3 && potentialDiag3 == potential13 && potentialDiag3
                                            != potential23)
                                    {
                                        satUpw3 = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potentialDiag3 && potentialDiag3 == potential23
                                            && potentialDiag3 != potential13)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace34, *isIt34);
                                    }
                                    else if (minPot == potential23 && potential23 == potential13)
                                    {
                                        satUpw3 = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                }

                                //                                    std::cout << "satUpw1 = " << satUpw1 << "satUpw3 = " << satUpw3 << "satUpw3 = "
                                //                                            << satUpw3 << "satUpw4 = " << satUpw4 << "\n";

                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw1) / viscosityNW;
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 2)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos3,
                                                *cellThree), satUpw3) / viscosityNW;
                            }
                        }
                    }
                    //nextIsIt is boundary
                    else
                    {
                        // center of face in global coordinates, i.e., the midpoint of edge 'nextIsIt'
                        const GlobalPosition& globalPosFace13 = nextIsIt->geometry().center();

                        // get boundary condition for boundary face (nextIsIt) center
                        BoundaryConditions::Flags nextIsItBCType = problem_.bctypeSat(globalPosFace13, *nextIsIt);

                        // 'isIt': Dirichlet boundary
                        if (isItBCType == BoundaryConditions::dirichlet)
                        {
                            if (nextIsItBCType == BoundaryConditions::dirichlet)
                            {
                                Scalar satUpw = 0;

                                Scalar potential11 =
                                        problem_.variables().potentialWetting(globalIdx, nextIndexInInside);
                                Scalar potential21 = problem_.variables().potentialWetting(globalIdx, indexInInside);
                                Scalar maxPot = std::max(potential11, potential21);
                                Scalar minPot = std::min(potential11, potential21);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potential11)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw = problem_.variables().saturation()[globalIdx];
                                    }
                                }
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityNW;
                            }
                            if (nextIsItBCType == BoundaryConditions::neumann)
                            {
                                Scalar satUpw = 0;

                                Scalar potential11 =
                                        problem_.variables().potentialWetting(globalIdx, nextIndexInInside);
                                Scalar potential21 = problem_.variables().potentialWetting(globalIdx, indexInInside);
                                Scalar maxPot = std::max(potential11, potential21);
                                Scalar minPot = std::min(potential11, potential21);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potential11)
                                    {
                                        satUpw = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace12, *isIt);
                                    }
                                }
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityNW;
                            }
                        }
                        if (isItBCType == BoundaryConditions::neumann)
                        {
                            if (nextIsItBCType == BoundaryConditions::dirichlet)
                            {
                                Scalar satUpw = 0;

                                Scalar potential11 =
                                        problem_.variables().potentialWetting(globalIdx, nextIndexInInside);
                                Scalar potential21 = problem_.variables().potentialWetting(globalIdx, indexInInside);
                                Scalar maxPot = std::max(potential11, potential21);
                                Scalar minPot = std::min(potential11, potential21);

                                if (std::abs(maxPot) >= std::abs(minPot))
                                {
                                    satUpw = problem_.variables().saturation()[globalIdx];
                                }
                                else
                                {
                                    if (minPot == potential11)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                    else if (minPot == potential21)
                                    {
                                        satUpw = problem_.variables().saturation()[globalIdx];
                                    }
                                    else if (minPot == potential21 && potential21 == potential11)
                                    {
                                        satUpw = problem_.dirichletSat(globalPosFace13, *nextIsIt);
                                    }
                                }
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krw(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = MaterialLaw::krn(problem_.spatialParameters().materialLawParams(globalPos,
                                                *eIt), satUpw) / viscosityNW;
                            }
                            if (nextIsItBCType == BoundaryConditions::neumann)
                            {
                                problem_.variables().upwindMobilitiesWetting(globalIdx, indexInInside, 0) = mobilityW;
                                problem_.variables().upwindMobilitiesNonwetting(globalIdx, indexInInside, 0)
                                        = mobilityNW;
                            }
                        }

//                                                std::cout << "upwind mobilities = " << problem_.variables().upwindMobilitiesWetting(globalIdx,
//                                                        indexInInside, 0) << ", " << problem_.variables().upwindMobilitiesNonwetting(globalIdx,
//                                                        indexInInside, 0) << "\n";
                    }
                }
            }
        }

        // initialize densities
        problem_.variables().densityWetting(globalIdx) = densityW;
        problem_.variables().densityNonwetting(globalIdx) = densityNW;

        // initialize viscosities
        problem_.variables().viscosityWetting(globalIdx) = viscosityW;
        problem_.variables().viscosityNonwetting(globalIdx) = viscosityNW;

        //initialize fractional flow functions
        problem_.variables().fracFlowFuncWetting(globalIdx) = mobilityW / (mobilityW + mobilityNW);
        problem_.variables().fracFlowFuncNonwetting(globalIdx) = mobilityNW / (mobilityW + mobilityNW);

        problem_.spatialParameters().update(satW, *eIt);
    }
    return;
}

}
// end of Dune namespace
#endif
