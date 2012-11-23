/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief The spatial parameters for the 2pDFM Problem which uses the
 *        twophase discrete fracture model.
 */
#ifndef EWOMS_TEST_2PDFM_SPATIAL_PARAMETERS_HH
#define EWOMS_TEST_2PDFM_SPATIAL_PARAMETERS_HH

#include <dumux/boxmodels/2pdfm/2pdfmmodel.hh>
#include <dumux/io/artmeshreader.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/spatialparams/boxspatialparams.hh>

namespace Ewoms
{

//forward declaration
template<class TypeTag>
class TwoPDFMSpatialParameters;

namespace Properties
{
// The spatial parameters TypeTag
NEW_TYPE_TAG(TwoPDFMSpatialParameters);

// Set the spatial parameters
SET_TYPE_PROP(TwoPDFMSpatialParameters, SpatialParams, Ewoms::TwoPDFMSpatialParameters<TypeTag>);

// Set the material Law
SET_PROP(TwoPDFMSpatialParameters, MaterialLaw)
{
private:
    // define the material law which is parameterized by effective
    // saturations
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef RegularizedBrooksCorey<Scalar> EffectiveLaw;
public:
    // define the material law parameterized by absolute saturations
    typedef EffToAbsLaw<EffectiveLaw> type;
};
}
/*!
 * \ingroup TwoPBoxModel
 * \ingroup BoxTestProblems
 * \brief The spatial parameters for the 2PDFMProblem which uses the
 *        twophase box model
 */
template<class TypeTag>
class TwoPDFMSpatialParameters : public BoxSpatialParams<TypeTag>
{

    template<int dim>
    struct FaceLayout
    {
        bool contains (Dune::GeometryType gt)
        {
            return gt.dim() == dim - 1;
        }
    };
    template<int dim>
    struct VertexLayout
    {
        bool contains (Dune::GeometryType gt)
        {
            return gt.dim() == 0;
        }
    };

    typedef BoxSpatialParams<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename Grid::ctype CoordScalar;

    typedef Dune::MultipleCodimMultipleGeomTypeMapper<GridView,VertexLayout> VertexMapper;
    typedef Dune::MultipleCodimMultipleGeomTypeMapper<GridView,FaceLayout> FaceMapper;

    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    typedef Dune::FieldVector<CoordScalar,dimWorld> GlobalPosition;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

public:
    //get the material law from the property system
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    TwoPDFMSpatialParameters(const GridView& gridView)
        : ParentType(gridView)
    {
        gridView_     = 0;
        facemapper_   = 0;
        vertexmapper_ = 0;

        setupFractureMatrixSoilParameters();
    }

    /**
     * Set the soil properties of fractures and domain.
     */
    void setupFractureMatrixSoilParameters()
    {
        Scalar mD = 1e-12 * 1e-3; //miliDarcy

        SwrF_    = 0.00;
        SwrM_    = 0.00;
        SnrF_    = 0.00;
        SnrM_    = 0.00;
        pdF_     = 1000; //2.5*1e4;
        pdM_     = 2000; //2.5*1e4;
        lambdaF_ = 2.0;
        lambdaM_ = 2.0;

        rockMatrixMaterialParams_.setSwr(SwrM_);
        rockMatrixMaterialParams_.setSnr(SnrM_);
        fractureMaterialParams_.setSwr(SwrF_);
        fractureMaterialParams_.setSnr(SnrF_);

        rockMatrixMaterialParams_.setPe(pdM_);
        rockMatrixMaterialParams_.setLambda(lambdaM_);
        fractureMaterialParams_.setPe(pdF_);
        fractureMaterialParams_.setLambda(lambdaF_);

        KMatrix_   = 1 * mD; //m^2
        KFracture_ = 1e5 * mD; //m^2

        porosityMatrix_   =    0.25;
        porosityFracture_ = 0.10;
        fractureWidth_    = 1e-2;
    }

    /*!
     * \brief Intrinsic permability
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     * \return Intrinsic permeability
     */
    Scalar intrinsicPermeability(const Element &element,
                                 const FVElementGeometry &fvGeometry,
                                 int scvIdx) const
    {
        return KMatrix_;
    }

    /*!
     * \brief Intrinsic permeability of fractures.
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     */
    Scalar intrinsicPermeabilityFracture(const Element &element,
                                         const FVElementGeometry &fvGeometry,
                                         int scvIdx) const
    {
        return KFracture_;
    }
    /*!
     * \brief Porosity
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     * \return Porosity
     */
    Scalar porosity(const Element &element,
                    const FVElementGeometry &fvGeometry,
                    int scvIdx) const
    { return porosityMatrix_; }

    /*!
     * \brief Porosity Fracture
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     * \return Porosity Fracture
     */
    Scalar porosityFracture(const Element &element,
                    const FVElementGeometry &fvGeometry,
                    int scvIdx) const
    {
        return porosityFracture_;
    }
    /*!
     * \brief Function for defining the parameters needed by constitutive relationships (kr-Sw, pc-Sw, etc.).
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     * \return the material parameters object
     */
    const MaterialLawParams& materialLawParams(const Element &element,
                                                const FVElementGeometry &fvGeometry,
                                                int scvIdx) const
    {
        return rockMatrixMaterialParams_;
    }

    /*!
     * \brief Function for defining the parameters needed by constitutive relationships (kr-Sw, pc-Sw, etc.).
     *
     * \param element The current element
     * \param fvGeometry The current finite volume geometry of the element
     * \param scvIdx The index of the sub-control volume.
     * \return the material parameters object of the Fracture
     */
    const MaterialLawParams& materialLawParamsFracture(const Element &element,
                                                    const FVElementGeometry &fvGeometry,
                                                    int scvIdx) const
    {
        int globalIdx = vertexmapper().map(element, scvIdx, dim);

        // be picky if called for non-fracture vertices
        assert(isVertexFracture(globalIdx));

        return fractureMaterialParams_;
    }

    /*!
     * \brief Checks whether vertex is a fracture.
     *
     * \param element The current element
     * \param localVertexIdx Vertex index to be checked
     */
    bool isVertexFracture(const Element &element, int localVertexIdx) const
    {
        if (inactivateFractures_)
        {
            return false;
        }
        int globalIdx = vertexmapper().map(element, localVertexIdx, dim);
        return isDuneFractureVertex_[globalIdx];
    }

    /*!
     * \brief Checks whether vertex is a fracture.
     *
     * \param globalIdx Vertex index to be checked
     */
    bool isVertexFracture(int globalIdx) const
    {
        if (inactivateFractures_)
        {
            return false;
        }
        return isDuneFractureVertex_[globalIdx];
    }

    /*!
     * \brief Checks whether element edge is a fracture.
     *
     * \param element The current element
     * \param localFaceIdx Face index to be checked
     */
    bool isEdgeFracture(const Element &element, int localFaceIdx) const
    {
        int globalIdx = facemapper().map(element, localFaceIdx, 1);
        return isDuneFractureEdge_[globalIdx];
    }

    /*!
     * \brief Returns the vertex mapper.
     */
    const VertexMapper &vertexmapper() const
    { return *vertexmapper_; }

    /*!
     * \brief Returns the face mapper.
     */
    const FaceMapper &facemapper() const
    { return *facemapper_; }

    /*!
     * \brief Returns the width of the fracture.
     *
     * \param globalFaceIdx Global face index of which the width is returned
     */
    Scalar fractureWidth(int globalFaceIdx) const
    {
        return fractureWidth_;
    }

    /*!
     * \brief Returns the width of the fracture.
     *
     * \param element The current element
     * \param globalFaceIdx Face index of which the width is returned
     */
    Scalar fractureWidth(const Element &element, int localFaceIdx) const
    {
        return fractureWidth_;
    }

    /*!
     * \brief Set the grid view.
     *
     * \param gv The new grid view to be set.
     */
    void setGridView(const GridView &gv)
     {
        delete gridView_;
        delete facemapper_;
        delete vertexmapper_;

        gridView_ = new GridView(gv);
        facemapper_ = new FaceMapper(gv);
        vertexmapper_ = new VertexMapper(gv);
     }

    /*!
     * \brief Set which vertices and edges are fractures.
     *
     * \param isDuneFractureVertex Vector of bools which indicates fracture vertices
     * \param isDuneFractureEdge Vector of bools which indicates fracture edges
     * \param inactivateFractures Deactivates fractures
     */
    const void setFractureBoolVectors(const std::vector<bool>& isDuneFractureVertex,
                                      const std::vector<bool>& isDuneFractureEdge,
                                      const std::vector<int>& fractureEdgesIdx,
                                      bool inactivateFractures)
    {
        isDuneFractureVertex_ = isDuneFractureVertex;
        isDuneFractureEdge_   = isDuneFractureEdge;
        inactivateFractures_  = inactivateFractures;
        fractureEdgesIdx_     = fractureEdgesIdx;
    }
    
    Scalar SwrF_;
    Scalar SwrM_;
    Scalar SnrF_;
    Scalar SnrM_;
    Scalar lambdaF_;
    Scalar lambdaM_;
    Scalar pdF_;
    Scalar pdM_;

private:
    MaterialLawParams materialParams_;

    Scalar KMatrix_;
    Scalar KFracture_;
    Scalar porosityMatrix_;
    Scalar porosityFracture_;

    Scalar fractureWidth_;

    MaterialLawParams fractureMaterialParams_;
    MaterialLawParams rockMatrixMaterialParams_;
    bool inactivateFractures_;

    std::vector<bool> isFracture_;
    std::vector<bool> isDuneFractureVertex_;
    std::vector<bool> isDuneFractureEdge_;
    std::vector<int>  fractureEdgesIdx_;

    VertexMapper *vertexmapper_;
    FaceMapper *facemapper_;
    GridView *gridView_;
};

} // end namespace
#endif // EWOMS_TEST_2PDFM_SPATIAL_PARAMETERS_HH