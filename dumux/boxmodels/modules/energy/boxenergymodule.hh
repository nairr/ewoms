// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2012 by Andreas Lauser                                    *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief Contains the classes required to consider energy as a
 *        conservation quantity in a multi-phase module.
 */
#ifndef DUMUX_BOX_ENERGY_MODULE_HH
#define DUMUX_BOX_ENERGY_MODULE_HH

#include <dumux/boxmodels/common/boxproperties.hh>

namespace Dumux {
namespace Properties {
NEW_PROP_TAG(Indices);
NEW_PROP_TAG(HeatConductionLaw);
NEW_PROP_TAG(HeatConductionLawParams);
}

/*!
 * \ingroup BoxEnergy
 * \class Dumux::BoxEnergyModule
 * \brief Provides the auxiliary methods required for consideration of the energy equation.
 */
template <class TypeTag, bool enableEnergy>
class BoxEnergyModule;

/*!
 * \copydoc Dumux::BoxEnergyModule
 */
template <class TypeTag>
class BoxEnergyModule<TypeTag, /*enableEnergy=*/false>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, EqVector) EqVector;
    typedef typename GET_PROP_TYPE(TypeTag, RateVector) RateVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename FluidSystem::ParameterCache ParameterCache;

public:
    /*!
     * \brief Register all run-time parameters for the energy module.
     */
    static void registerParameters()
    { }

    /*!
     * \brief Returns the name of a primary variable or an empty
     *        string if the specified primary variable index does not belong to
     *        the energy module.
     */
    static std::string primaryVarName(int pvIdx)
    { return ""; }

    /*!
     * \brief Returns the name of an equation or an empty
     *        string if the specified equation index does not belong to
     *        the energy module.
     */
    static std::string eqName(int eqIdx)
    { return ""; }

    /*!
     * \brief Returns the relative weight of a primary variable for
     *        calculating relative errors.
     */
    static Scalar primaryVarWeight(const Model &model, int globalVertexIdx, int pvIdx)
    { return -1; }

    /*!
     * \brief Returns the relative weight of a equation.
     */
    static Scalar eqWeight(const Model &model, int globalVertexIdx, int eqIdx)
    { return -1; }

    /*!
     * \brief Given a fluid state, set the temperature in the primary variables
     */
    template <class FluidState>
    static void setPriVarTemperatures(PrimaryVariables &priVars, const FluidState &fs)
    { }

    /*!
     * \brief Given a fluid state, set the enthalpy rate which emerges
     *        from a volumetric rate.
     */
    template <class FluidState>
    static void setEnthalpyRate(RateVector &rateVec,
                                const FluidState &fluidState,
                                int phaseIdx,
                                Scalar volume)
    { }

    /*!
     * \brief Add the rate of the enthalpy flux to a rate vector.
     */
    static void setEnthalpyRate(RateVector &rateVec, Scalar rate)
    { }

    /*!
     * \brief Add the rate of the enthalpy flux to a rate vector.
     */
    static void addToEnthalpyRate(RateVector &rateVec, Scalar rate)
    { }

    /*!
     * \brief Add the rate of the conductive heat flux to a rate vector.
     */
    static Scalar heatConductionRate(const FluxVariables &fluxVars)
    { return 0.0; }

    /*!
     * \brief Add the energy storage term for a fluid phase to an equation vector
     */
    static void addPhaseStorage(EqVector &storage, const VolumeVariables &volVars, int phaseIdx)
    { }

    /*!
     * \brief Add the energy storage term for a fluid phase to an equation vector
     */
    static void addSolidHeatStorage(EqVector &storage, const VolumeVariables &volVars)
    { }

    /*!
     * \brief Evaluates the advective energy fluxver a face of a
     *        subcontrol volume and adds the result in the flux vector.
     *
     * This method is called by compute flux (base class)
     */
    template <class Context>
    static void addAdvectiveFlux(RateVector &flux,
                                 const Context &context,
                                 int spaceIdx,
                                 int timeIdx)
    { }

    /*!
     * \brief Adds the diffusive heat flux to the flux vector over
     *        the face of a sub-control volume.
     *
     * This method is called by compute flux (base class)
     */
    template <class Context>
    static void addDiffusiveFlux(RateVector &flux,
                                 const Context &context,
                                 int spaceIdx,
                                 int timeIdx)
    { }
};

/*!
 * \copydoc Dumux::BoxEnergyModule
 */
template <class TypeTag>
class BoxEnergyModule<TypeTag, /*enableEnergy=*/true>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, EqVector) EqVector;
    typedef typename GET_PROP_TYPE(TypeTag, RateVector) RateVector;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, FluxVariables) FluxVariables;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename FluidSystem::ParameterCache ParameterCache;

    enum { numPhases = FluidSystem::numPhases };
    enum { energyEqIdx = Indices::energyEqIdx };
    enum { temperatureIdx = Indices::temperatureIdx };

public:
    /*!
     * \brief Register all run-time parameters for the energy module.
     */
    static void registerParameters()
    { }

    /*!
     * \brief Returns the name of a primary variable or an empty
     *        string if the specified primary variable index does not belong to
     *        the energy module.
     */
    static std::string primaryVarName(int pvIdx)
    {
        if (pvIdx == temperatureIdx)
            return "temperature";
        return "";
    }

    /*!
     * \brief Returns the name of an equation or an empty
     *        string if the specified equation index does not belong to
     *        the energy module.
     */
    static std::string eqName(int eqIdx)
    {
        if (eqIdx == energyEqIdx)
            return "energy";
        return "";
    }

    /*!
     * \brief Returns the relative weight of a primary variable for
     *        calculating relative errors.
     */
    static Scalar primaryVarWeight(const Model &model, int globalVertexIdx, int pvIdx)
    {
        if (pvIdx != temperatureIdx)
            return -1;

        // make the weight of the temperature primary variable inversly proportional to its value
        return std::max(1.0/1000, 1.0/model.solution(/*timeIdx=*/0)[globalVertexIdx][temperatureIdx]);
    }

    /*!
     * \brief Returns the relative weight of a equation.
     */
    static Scalar eqWeight(const Model &model, int globalVertexIdx, int eqIdx)
    {
        if (eqIdx != energyEqIdx)
            return -1;

        // approximate heat capacity of 1kg of air
        return 1.0/1.0035e3;
    }

    /*!
     * \brief Add the rate of the enthalpy flux to a rate vector.
     */
    static void setEnthalpyRate(RateVector &rateVec, Scalar rate)
    { rateVec[energyEqIdx] = rate; }

    /*!
     * \brief Add the rate of the enthalpy flux to a rate vector.
     */
    static void addToEnthalpyRate(RateVector &rateVec, Scalar rate)
    { rateVec[energyEqIdx] += rate; }

    /*!
     * \brief Add the rate of the conductive heat flux to a rate vector.
     */
    static Scalar heatConductionRate(const FluxVariables &fluxVars)
    {
        return
            - fluxVars.temperatureGradNormal()
            * fluxVars.heatConductivity();
    }

    /*!
     * \brief Given a fluid state, set the enthalpy rate which emerges
     *        from a volumetric rate.
     */
    template <class FluidState>
    static void setEnthalpyRate(RateVector &rateVec,
                                const FluidState &fluidState,
                                int phaseIdx,
                                Scalar volume)
    {
        rateVec[energyEqIdx] =
            volume
            * fluidState.density(phaseIdx)
            * fluidState.enthalpy(phaseIdx);
    }

    /*!
     * \brief Given a fluid state, set the temperature in the primary variables
     */
    template <class FluidState>
    static void setPriVarTemperatures(PrimaryVariables &priVars, const FluidState &fs)
    {
        priVars[temperatureIdx] = fs.temperature(/*phaseIdx=*/0);
#ifndef NDEBUG
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            assert(fs.temperature(/*phaseIdx=*/0) == fs.temperature(phaseIdx));
        }
#endif
    }

    /*!
     * \brief Add the energy storage term for a fluid phase to an equation vector
     */
    static void addPhaseStorage(EqVector &storage, const VolumeVariables &volVars, int phaseIdx)
    {
        const auto &fs = volVars.fluidState();
        storage[energyEqIdx] +=
            fs.density(phaseIdx)
            * fs.internalEnergy(phaseIdx)
            * fs.saturation(phaseIdx)
            * volVars.porosity();
    }

    /*!
     * \brief Add the energy storage term for a fluid phase to an equation vector
     */
    static void addSolidHeatStorage(EqVector &storage, const VolumeVariables &volVars)
    {
        storage[energyEqIdx] +=
            volVars.heatCapacitySolid()
            * volVars.fluidState().temperature(/*phaseIdx=*/0);
    }

    /*!
     * \brief Evaluates the advective energy fluxver a face of a
     *        subcontrol volume and adds the result in the flux vector.
     *
     * This method is called by compute flux (base class)
     */
    template <class Context>
    static void addAdvectiveFlux(RateVector &flux,
                                 const Context &context,
                                 int spaceIdx,
                                 int timeIdx)
    {
        const auto &fluxVars = context.fluxVars(spaceIdx, timeIdx);
        const auto &evalPointFluxVars = context.evalPointFluxVars(spaceIdx, timeIdx);

        // advective heat flux in all phases
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            if (!context.model().phaseIsConsidered(phaseIdx))
                continue;

            // vertex data of the upstream and the downstream vertices
            const VolumeVariables &up = context.volVars(evalPointFluxVars.upstreamIndex(phaseIdx), timeIdx);

            flux[energyEqIdx] +=
                fluxVars.volumeFlux(phaseIdx)
                * up.fluidState().enthalpy(phaseIdx)
                * up.fluidState().density(phaseIdx);
        }
    }

    /*!
     * \brief Adds the diffusive heat flux to the flux vector over
     *        the face of a sub-control volume.
     *
     * This method is called by compute flux (base class)
     */
    template <class Context>
    static void addDiffusiveFlux(RateVector &flux,
                                 const Context &context,
                                 int spaceIdx,
                                 int timeIdx)
    {
        const auto &fluxVars = context.fluxVars(spaceIdx, timeIdx);

        // diffusive heat flux
        flux[energyEqIdx] +=
            -fluxVars.temperatureGradNormal()
            * fluxVars.heatConductivity();
    }
};

/*!
 * \ingroup BoxEnergy
 * \class Dumux::BoxEnergyIndices
 *
 * \brief Provides the indices required for consideration of the energy equation.
 */
template <int PVOffset, bool enableEnergy>
struct BoxEnergyIndices;

/*!
 * \copydoc Dumux::BoxEnergyIndices
 */
template <int PVOffset>
struct BoxEnergyIndices<PVOffset, /*enableEnergy=*/false>
{
protected:
    enum { numEq_ = 0 };
};

/*!
 * \copydoc Dumux::BoxEnergyIndices
 */
template <int PVOffset>
struct BoxEnergyIndices<PVOffset, /*enableEnergy=*/true>
{
    //! The index of the primary variable representing temperature
    enum { temperatureIdx = PVOffset };

    //! The index of the equation representing the conservation of energy
    enum { energyEqIdx = PVOffset };

protected:
    enum { numEq_ = 1 };
};

/*!
 * \ingroup BoxEnergy
 * \class Dumux::BoxEnergyVolumeVariables
 *
 * \brief Provides the volumetric quantities required for the energy equation.
 */
template <class TypeTag, bool enableEnergy>
class BoxEnergyVolumeVariables;

/*!
 * \copydoc Dumux::BoxEnergyVolumeVariables
 */
template <class TypeTag>
class BoxEnergyVolumeVariables<TypeTag, /*enableEnergy=*/false>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename FluidSystem::ParameterCache ParameterCache;

public:
    /*!
     * \brief Returns the total heat capacity \f$\mathrm{[J/(K*m^3]}\f$ of the rock matrix in
     *        the sub-control volume.
     */
    Scalar heatCapacitySolid() const
    { DUNE_THROW(Dune::InvalidStateException, "Method heatCapacitySolid() does not make sense for isothermal models"); }

    /*!
     * \brief Returns the total conductivity capacity
     *        \f$\mathrm{[W/m^2 / (K/m)]}\f$ of the rock matrix in the
     *        sub-control volume.
     */
    Scalar heatConductivity() const
    { DUNE_THROW(Dune::InvalidStateException, "Method heatConductivity() does not make sense for isothermal models"); }

protected:
    /*!
     * \brief Update the temperatures of the fluids of a fluid state.
     */
    template <class FluidState, class Context>
    static void updateTemperatures_(FluidState &fluidState, const Context &context, int spaceIdx, int timeIdx)
    { fluidState.setTemperature(context.problem().temperature(context, spaceIdx, timeIdx)); }


    /*!
     * \brief Update the quantities required to calculate
     *        energy fluxes.
     */
    template <class FluidState>
    void update_(FluidState &fs,
                 ParameterCache &paramCache,
                 const ElementContext &elemCtx,
                 int scvIdx,
                 int timeIdx)
    { }
};

/*!
 * \copydoc Dumux::BoxEnergyVolumeVariables
 */
template <class TypeTag>
class BoxEnergyVolumeVariables<TypeTag, /*enableEnergy=*/true>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, HeatConductionLaw) HeatConductionLaw;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename FluidSystem::ParameterCache ParameterCache;

    enum { numPhases = FluidSystem::numPhases };
    enum { energyEqIdx = Indices::energyEqIdx };
    enum { temperatureIdx = Indices::temperatureIdx };

protected:
    /*!
     * \brief Update the temperatures of the fluids of a fluid state.
     */
    template <class FluidState, class Context>
    static void updateTemperatures_(FluidState &fluidState, const Context &context, int spaceIdx, int timeIdx)
    {
        fluidState.setTemperature(context.primaryVars(spaceIdx, timeIdx)[temperatureIdx]);
    }


    /*!
     * \brief Update the quantities required to calculate
     *        energy fluxes.
     */
    template <class FluidState>
    void update_(FluidState &fs,
                 ParameterCache &paramCache,
                 const ElementContext &elemCtx,
                 int scvIdx,
                 int timeIdx)
    {
        // set the specific enthalpies of the fluids
        for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
            if (!elemCtx.model().phaseIsConsidered(phaseIdx))
                continue;

            fs.setEnthalpy(phaseIdx,
                           FluidSystem::enthalpy(fs, paramCache, phaseIdx));
        }

        // compute and set the heat capacity of the solid phase
        const auto &problem = elemCtx.problem();
        const auto &heatCondParams = problem.heatConductionParams(elemCtx, scvIdx, timeIdx);

        heatCapacitySolid_ = problem.heatCapacitySolid(elemCtx, scvIdx, timeIdx);
        heatConductivity_ = HeatConductionLaw::heatConductivity(heatCondParams, fs);

        Valgrind::CheckDefined(heatCapacitySolid_);
        Valgrind::CheckDefined(heatConductivity_);
    }

public:
    /*!
     * \brief Returns the total heat capacity \f$\mathrm{[J/(K*m^3]}\f$ of the rock matrix in
     *        the sub-control volume.
     */
    Scalar heatCapacitySolid() const
    { return heatCapacitySolid_; }

    /*!
     * \brief Returns the total conductivity capacity
     *        \f$\mathrm{[W/m^2 / (K/m)]}\f$ of the rock matrix in the
     *        sub-control volume.
     */
    Scalar heatConductivity() const
    { return heatConductivity_; }

private:
    Scalar heatCapacitySolid_;
    Scalar heatConductivity_;
};

/*!
 * \ingroup BoxEnergy
 * \class Dumux::BoxEnergyFluxVariables
 *
 * \brief Provides the quantities required to calculate energy fluxes.
 */
template <class TypeTag, bool enableEnergy>
class BoxEnergyFluxVariables;

/*!
 * \copydoc Dumux::BoxEnergyFluxVariables
 */
template <class TypeTag>
class BoxEnergyFluxVariables<TypeTag, /*enableEnergy=*/false>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;

protected:
    /*!
     * \brief Update the quantities required to calculate
     *        energy fluxes.
     */
    void update_(const ElementContext &elemCtx, int scvfIdx, int timeIdx)
    { }

    template <class Context, class FluidState>
    void updateBoundary_(const Context &context,
                         int bfIdx,
                         int timeIdx,
                         const FluidState &fs)
    { }

public:
    /*!
     * \brief The temperature gradient times the face normal [K m^2 / m]
     */
    Scalar temperatureGradNormal() const
    { DUNE_THROW(Dune::InvalidStateException, "Method temperatureGradNormal() does not make sense for isothermal models"); }

    /*!
     * \brief The total heat conductivity at the face \f$\mathrm{[W/m^2 / (K/m)]}\f$
     */
    Scalar heatConductivity() const
    { DUNE_THROW(Dune::InvalidStateException, "Method heatConductivity() does not make sense for isothermal models"); }
};

/*!
 * \copydoc Dumux::BoxEnergyFluxVariables
 */
template <class TypeTag>
class BoxEnergyFluxVariables<TypeTag, /*enableEnergy=*/true>
{
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    enum { dimWorld = GridView::dimensionworld };
    typedef Dune::FieldVector<Scalar, dimWorld> DimVector;

protected:
    /*!
     * \brief Update the quantities required to calculate
     *        energy fluxes.
     */
    void update_(const ElementContext &elemCtx, int scvfIdx, int timeIdx)
    {
        const auto &scvf = elemCtx.fvElemGeom(timeIdx).subContVolFace[scvfIdx];
        // calculate temperature gradient using finite element
        // gradients
        DimVector temperatureGrad;
        DimVector tmp;
        temperatureGrad = Scalar(0.0);
        for (int scvIdx = 0; scvIdx < elemCtx.numScv(); scvIdx++)
        {
            const auto &feGrad = scvf.grad[scvIdx];
            const auto &volVars = elemCtx.volVars(scvIdx, timeIdx);

            tmp = feGrad;
            tmp *= volVars.fluidState().temperature(/*phaseIdx=*/0);
            temperatureGrad += tmp;
        }

        // scalar product of temperature gradient and scvf normal
        temperatureGradNormal_ = scvf.normal*temperatureGrad;

        const auto &fluxVars = elemCtx.fluxVars(scvfIdx, timeIdx);
        const auto &volVarsInside = elemCtx.volVars(fluxVars.insideIndex(), timeIdx);
        const auto &volVarsOutside = elemCtx.volVars(fluxVars.outsideIndex(), timeIdx);

        // arithmetic mean
        heatConductivity_ =
            0.5 * (volVarsInside.heatConductivity()
                   +
                   volVarsOutside.heatConductivity());
        Valgrind::CheckDefined(heatConductivity_);
    }

    template <class Context, class FluidState>
    void updateBoundary_(const Context &context,
                         int bfIdx,
                         int timeIdx,
                         const FluidState &fs)
    {
        const auto &fvElemGeom = context.fvElemGeom(timeIdx);
        const auto &scvf = fvElemGeom.boundaryFace[bfIdx];

        const auto &elemCtx = context.elemContext();
        int insideScvIdx = scvf.i;
        const auto &insideScv = elemCtx.fvElemGeom(timeIdx).subContVol[insideScvIdx];

        const auto &volVarsInside = elemCtx.volVars(insideScvIdx, timeIdx);
        const auto &fsInside = volVarsInside.fluidState();


        // distance between the center of the SCV and center of the boundary face
        DimVector distVec = scvf.ipGlobal;
        distVec -= context.element().geometry().global(insideScv.localGeometry->center());

        const DimVector &n = scvf.normal;
        Scalar dist = (distVec * n)/n.two_norm();

        // if the following assertation triggers, the center of the
        // center of the interior SCV was not inside the element!
        assert(dist > 0);

        // calculate the temperature gradient using two-point gradient
        // appoximation
        temperatureGradNormal_ =
            (fs.temperature(/*phaseIdx=*/0) - fsInside.temperature(/*phaseIdx=*/0))
            / dist;

        // take the value for heat conductivity from the interior finite volume
        heatConductivity_ = volVarsInside.heatConductivity();
    }

public:
    /*!
     * \brief The temperature gradient times the face normal [K m^2 / m]
     */
    Scalar temperatureGradNormal() const
    { return temperatureGradNormal_; }

    /*!
     * \brief The total heat conductivity at the face \f$\mathrm{[W/m^2 / (K/m)]}\f$
     */
    Scalar heatConductivity() const
    { return heatConductivity_; }

private:
    Scalar temperatureGradNormal_;
    Scalar heatConductivity_;
};

} // end namepace

#endif
