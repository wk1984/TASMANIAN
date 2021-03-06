/*
 * Copyright (c) 2017, Miroslav Stoyanov
 *
 * This file is part of
 * Toolkit for Adaptive Stochastic Modeling And Non-Intrusive ApproximatioN: TASMANIAN
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * UT-BATTELLE, LLC AND THE UNITED STATES GOVERNMENT MAKE NO REPRESENTATIONS AND DISCLAIM ALL WARRANTIES, BOTH EXPRESSED AND IMPLIED.
 * THERE ARE NO EXPRESS OR IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, OR THAT THE USE OF THE SOFTWARE WILL NOT INFRINGE ANY PATENT,
 * COPYRIGHT, TRADEMARK, OR OTHER PROPRIETARY RIGHTS, OR THAT THE SOFTWARE WILL ACCOMPLISH THE INTENDED RESULTS OR THAT THE SOFTWARE OR ITS USE WILL NOT RESULT IN INJURY OR DAMAGE.
 * THE USER ASSUMES RESPONSIBILITY FOR ALL LIABILITIES, PENALTIES, FINES, CLAIMS, CAUSES OF ACTION, AND COSTS AND EXPENSES, CAUSED BY, RESULTING FROM OR ARISING OUT OF,
 * IN WHOLE OR IN PART THE USE, STORAGE OR DISPOSAL OF THE SOFTWARE.
 */

#ifndef __TASMANIAN_SPARSE_GRID_GLOBAL_CPP
#define __TASMANIAN_SPARSE_GRID_GLOBAL_CPP

#include "tsgGridGlobal.hpp"

#include "tsgHiddenExternals.hpp"

#ifdef Tasmanian_ENABLE_CUDA
#include <cuda_runtime_api.h>
#include <cuda.h>
#endif // Tasmanian_ENABLE_CUDA

#include "tsgCudaMacros.hpp"

namespace TasGrid{

GridGlobal::GridGlobal() : num_dimensions(0), num_outputs(0), alpha(0.0), beta(0.0){}
GridGlobal::~GridGlobal(){}

void GridGlobal::write(std::ofstream &ofs) const{
    using std::endl;

    ofs << std::scientific; ofs.precision(17);
    ofs << num_dimensions << " " << num_outputs << " " << alpha << " " << beta << endl;
    if (num_dimensions > 0){
        ofs << OneDimensionalMeta::getIORuleString(rule) << endl;
        if (rule == rule_customtabulated){
            custom.write(ofs);
        }
        tensors.write(ofs);
        active_tensors.write(ofs);
        if (!active_w.empty()){
            ofs << active_w[0];
            for(int i=1; i<active_tensors.getNumIndexes(); i++) ofs << " " << active_w[i];
            ofs << endl;
        }
        if (points.empty()){
            ofs << "0" << endl;
        }else{
            ofs << "1 ";
            points.write(ofs);
        }
        if (needed.empty()){
            ofs << "0" << endl;
        }else{
            ofs << "1 ";
            needed.write(ofs);
        }
        ofs << max_levels[0];
        for(int j=1; j<num_dimensions; j++){
            ofs << " " << max_levels[j];
        }
        ofs << endl;
        if (num_outputs > 0) values.write(ofs);
        if (!updated_tensors.empty()){
            ofs << "1" << endl;
            updated_tensors.write(ofs);
            updated_active_tensors.write(ofs);
            ofs << updated_active_w[0];
            for(int i=1; i<updated_active_tensors.getNumIndexes(); i++){
                ofs << " " << updated_active_w[i];
            }
        }else{
            ofs << "0";
        }
        ofs << endl;
    }
}
void GridGlobal::writeBinary(std::ofstream &ofs) const{
    int num_dim_out[2];
    num_dim_out[0] = num_dimensions;
    num_dim_out[1] = num_outputs;
    ofs.write((char*) num_dim_out, 2*sizeof(int));
    double alpha_beta[2];
    alpha_beta[0] = alpha;
    alpha_beta[1] = beta;
    ofs.write((char*) alpha_beta, 2*sizeof(double));
    if (num_dimensions > 0){
        num_dim_out[0] = OneDimensionalMeta::getIORuleInt(rule);
        ofs.write((char*) num_dim_out, sizeof(int));
        if (rule == rule_customtabulated){
            custom.writeBinary(ofs);
        }
        tensors.writeBinary(ofs);
        active_tensors.writeBinary(ofs);
        if (!active_w.empty()) ofs.write((char*) (active_w.data()), active_tensors.getNumIndexes() * sizeof(int));
        char flag;
        if (points.empty()){
            flag = 'n'; ofs.write(&flag, sizeof(char));
        }else{
            flag = 'y'; ofs.write(&flag, sizeof(char));
            points.writeBinary(ofs);
        }
        if (needed.empty()){
            flag = 'n'; ofs.write(&flag, sizeof(char));
        }else{
            flag = 'y'; ofs.write(&flag, sizeof(char));
            needed.writeBinary(ofs);
        }
        ofs.write((char*) max_levels.data(), num_dimensions * sizeof(int));

        if (num_outputs > 0) values.writeBinary(ofs);
        if (!updated_tensors.empty()){
            flag = 'y'; ofs.write(&flag, sizeof(char));
            updated_tensors.writeBinary(ofs);
            updated_active_tensors.writeBinary(ofs);
            ofs.write((char*) (updated_active_w.data()), updated_active_tensors.getNumIndexes() * sizeof(int));
        }else{
            flag = 'n'; ofs.write(&flag, sizeof(char));
        }
    }
}
void GridGlobal::read(std::ifstream &ifs){
    reset(true); // true deletes any custom rule
    ifs >> num_dimensions >> num_outputs >> alpha >> beta;
    if (num_dimensions > 0){
        int flag;
        std::string T;
        ifs >> T;
        rule = OneDimensionalMeta::getIORuleString(T.c_str());
        if (rule == rule_customtabulated) custom.read(ifs);
        tensors.read(ifs);
        active_tensors.read(ifs);
        active_w.resize(active_tensors.getNumIndexes());
        for(auto &aw : active_w) ifs >> aw;

        ifs >> flag; if (flag == 1) points.read(ifs);
        ifs >> flag; if (flag == 1) needed.read(ifs);

        max_levels.resize(num_dimensions);
        for(auto &m : max_levels) ifs >> m;

        if (num_outputs > 0) values.read(ifs);
        ifs >> flag;

        int oned_max_level;
        if (flag == 1){
            updated_tensors.read(ifs);
            oned_max_level = *std::max_element(updated_tensors.getVector()->begin(), updated_tensors.getVector()->end());

            updated_active_tensors.read(ifs);

            updated_active_w.resize(updated_active_tensors.getNumIndexes());
            for(auto &aw : updated_active_w) ifs >> aw;
        }else{
            oned_max_level = *std::max_element(max_levels.begin(), max_levels.end());
        }

        wrapper.load(custom, oned_max_level, rule, alpha, beta);

        recomputeTensorRefs((points.empty()) ? needed : points);
    }
}

void GridGlobal::readBinary(std::ifstream &ifs){
    reset(true); // true deletes any custom rule
    int num_dim_out[2];
    ifs.read((char*) num_dim_out, 2*sizeof(int));
    num_dimensions = num_dim_out[0];
    num_outputs = num_dim_out[1];
    double alpha_beta[2];
    ifs.read((char*) alpha_beta, 2*sizeof(double));
    alpha = alpha_beta[0];
    beta = alpha_beta[1];
    if (num_dimensions > 0){
        ifs.read((char*) num_dim_out, sizeof(int));
        rule = OneDimensionalMeta::getIORuleInt(num_dim_out[0]);
        if (rule == rule_customtabulated) custom.readBinary(ifs);

        tensors.readBinary(ifs);
        active_tensors.readBinary(ifs);
        active_w.resize(active_tensors.getNumIndexes());
        if (!active_w.empty()) ifs.read((char*) (active_w.data()), active_tensors.getNumIndexes() * sizeof(int));

        char flag;
        ifs.read((char*) &flag, sizeof(char)); if (flag == 'y') points.readBinary(ifs);
        ifs.read((char*) &flag, sizeof(char)); if (flag == 'y') needed.readBinary(ifs);

        max_levels.resize(num_dimensions);
        ifs.read((char*) max_levels.data(), num_dimensions * sizeof(int));

        if (num_outputs > 0) values.readBinary(ifs);

        ifs.read((char*) &flag, sizeof(char));
        int oned_max_level;
        if (flag == 'y'){
            updated_tensors.readBinary(ifs);
            oned_max_level = *std::max_element(updated_tensors.getVector()->begin(), updated_tensors.getVector()->end());

            updated_active_tensors.readBinary(ifs);

            updated_active_w.resize(updated_active_tensors.getNumIndexes());
            ifs.read((char*) updated_active_w.data(), updated_active_w.size() * sizeof(int));
        }else{
            oned_max_level = *std::max_element(max_levels.begin(), max_levels.end());
        }

        wrapper.load(custom, oned_max_level, rule, alpha, beta);

        recomputeTensorRefs((points.empty()) ? needed : points);
    }
}

void GridGlobal::reset(bool includeCustom){
    clearAccelerationData();
    tensor_refs = std::vector<std::vector<int>>();
    wrapper = OneDimensionalWrapper();
    tensors = MultiIndexSet();
    active_tensors = MultiIndexSet();
    active_w = std::vector<int>();
    points = MultiIndexSet();
    needed = MultiIndexSet();
    values = StorageSet();
    updated_tensors = MultiIndexSet();
    updated_active_tensors = MultiIndexSet();
    updated_active_w = std::vector<int>();
    if (includeCustom) custom = CustomTabulated();
    num_dimensions = num_outputs = 0;
}

void GridGlobal::clearRefinement(){
    needed = MultiIndexSet();
    updated_tensors = MultiIndexSet();
    updated_active_tensors = MultiIndexSet();
    updated_active_w = std::vector<int>();
}

void GridGlobal::selectTensors(int depth, TypeDepth type, const std::vector<int> &anisotropic_weights, TypeOneDRule crule, MultiIndexSet &tset) const{
    if ((type == type_level) || (type == type_tensor) || (type == type_hyperbolic)){ // no need to know exactness
        MultiIndexManipulations::selectTensors(depth, type, [&](int l) -> long long{ return l; }, anisotropic_weights, tset);
    }else{ // work with exactness specific to the rule
        if (crule == rule_customtabulated){
            if ((type == type_qptotal) || (type == type_qptensor) || (type == type_qpcurved) || (type == type_qphyperbolic)){
                MultiIndexManipulations::selectTensors(depth, type, [&](int l) -> long long{ return custom.getQExact(l); }, anisotropic_weights, tset);
            }else{
                MultiIndexManipulations::selectTensors(depth, type, [&](int l) -> long long{ return custom.getIExact(l); }, anisotropic_weights, tset);
            }
        }else{ // using regular OneDimensionalMeta
            if ((type == type_qptotal) || (type == type_qptensor) || (type == type_qpcurved) || (type == type_qphyperbolic)){
                MultiIndexManipulations::selectTensors(depth, type, [&](int l) -> long long{ return OneDimensionalMeta::getQExact(l, crule); }, anisotropic_weights, tset);
            }else{
                MultiIndexManipulations::selectTensors(depth, type, [&](int l) -> long long{ return OneDimensionalMeta::getIExact(l, crule); }, anisotropic_weights, tset);
            }
        }
    }
}
void GridGlobal::recomputeTensorRefs(const MultiIndexSet &work){
    int nz_weights = active_tensors.getNumIndexes();
    tensor_refs.resize((size_t) nz_weights);
    if (OneDimensionalMeta::isNonNested(rule)){
        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<nz_weights; i++)
            MultiIndexManipulations::referencePoints<false>(active_tensors.getIndex(i), wrapper, work, tensor_refs[i]);
    }else{
        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<nz_weights; i++)
            MultiIndexManipulations::referencePoints<true>(active_tensors.getIndex(i), wrapper, work, tensor_refs[i]);
    }
}

void GridGlobal::makeGrid(int cnum_dimensions, int cnum_outputs, int depth, TypeDepth type, TypeOneDRule crule, const std::vector<int> &anisotropic_weights, double calpha, double cbeta, const char* custom_filename, const std::vector<int> &level_limits){
    if (crule == rule_customtabulated){
        custom.read(custom_filename);
    }

    MultiIndexSet tset(cnum_dimensions);
    selectTensors(depth, type, anisotropic_weights, crule, tset);

    if (!level_limits.empty()) MultiIndexManipulations::removeIndexesByLimit(level_limits, tset);

    setTensors(tset, cnum_outputs, crule, calpha, cbeta);
}
void GridGlobal::copyGrid(const GridGlobal *global){
    custom = CustomTabulated();
    if (global->rule == rule_customtabulated) custom = global->custom;

    MultiIndexSet tset = global->tensors;
    setTensors(tset, global->num_outputs, global->rule, global->alpha, global->beta);

    if ((num_outputs > 0) && (!(global->points.empty()))){ // if there are values inside the source object
        loadNeededPoints(global->values.getValues(0));
    }

    // if there is an active refinement awaiting values
    if (!(global->updated_tensors.empty())){
        updated_tensors = global->updated_tensors;
        updated_active_tensors = global->updated_active_tensors;
        updated_active_w = global->updated_active_w;

        needed = global->needed;

        wrapper.load(custom, global->wrapper.getNumLevels(), rule, alpha, beta);
    }
}

void GridGlobal::setTensors(MultiIndexSet &tset, int cnum_outputs, TypeOneDRule crule, double calpha, double cbeta){
    reset(false);
    num_dimensions = tset.getNumDimensions();
    num_outputs = cnum_outputs;
    rule = crule;
    alpha = calpha;  beta = cbeta;

    tensors = std::move(tset);

    int max_level;
    MultiIndexManipulations::getMaxIndex(tensors, max_levels, max_level);

    wrapper.load(custom, max_level, rule, alpha, beta);

    std::vector<int> tensors_w;
    MultiIndexManipulations::computeTensorWeights(tensors, tensors_w);
    MultiIndexManipulations::createActiveTensors(tensors, tensors_w, active_tensors);

    active_w.reserve(active_tensors.getNumIndexes());
    for(auto w : tensors_w) if (w != 0) active_w.push_back(w);

    if (OneDimensionalMeta::isNonNested(rule)){
        MultiIndexManipulations::generateNonNestedPoints(active_tensors, wrapper, needed);
    }else{
        MultiIndexManipulations::generateNestedPoints(tensors, [&](int l) -> int{ return wrapper.getNumPoints(l); }, needed);
    }

    recomputeTensorRefs(needed);

    if (num_outputs == 0){
        points = std::move(needed);
        needed = MultiIndexSet();
    }else{
        values.resize(num_outputs, needed.getNumIndexes());
    }
}

void GridGlobal::proposeUpdatedTensors(){
    int max_level = *std::max_element(updated_tensors.getVector()->begin(), updated_tensors.getVector()->end());
    wrapper.load(custom, max_level, rule, alpha, beta);

    std::vector<int> updates_tensor_w;
    MultiIndexManipulations::computeTensorWeights(updated_tensors, updates_tensor_w);
    MultiIndexManipulations::createActiveTensors(updated_tensors, updates_tensor_w, updated_active_tensors);

    updated_active_w.reserve(updated_active_tensors.getNumIndexes());
    for(auto w : updates_tensor_w) if (w != 0) updated_active_w.push_back(w);

    MultiIndexSet new_points;
    if (OneDimensionalMeta::isNonNested(rule)){
        MultiIndexManipulations::generateNonNestedPoints(updated_active_tensors, wrapper, new_points);
    }else{
        MultiIndexManipulations::generateNestedPoints(updated_tensors, [&](int l) -> int{ return wrapper.getNumPoints(l); }, new_points);
    }

    new_points.diffSets(points, needed);
}

void GridGlobal::updateGrid(int depth, TypeDepth type, const std::vector<int> &anisotropic_weights, const std::vector<int> &level_limits){
    if ((num_outputs == 0) || points.empty()){
        makeGrid(num_dimensions, num_outputs, depth, type, rule, anisotropic_weights, alpha, beta, 0, level_limits);
    }else{
        clearRefinement();

        updated_tensors.setNumDimensions(num_dimensions);
        selectTensors(depth, type, anisotropic_weights, rule, updated_tensors);

        if (!level_limits.empty()) MultiIndexManipulations::removeIndexesByLimit(level_limits, updated_tensors);

        MultiIndexSet new_tensors;
        updated_tensors.diffSets(tensors, new_tensors);

        if (!new_tensors.empty()){
            updated_tensors.addMultiIndexSet(tensors);
            proposeUpdatedTensors();
        }
    }
}

int GridGlobal::getNumDimensions() const{ return num_dimensions; }
int GridGlobal::getNumOutputs() const{ return num_outputs; }
TypeOneDRule GridGlobal::getRule() const{ return rule; }
const char* GridGlobal::getCustomRuleDescription() const{ return (custom.getNumLevels() > 0) ? custom.getDescription() : "";  }

double GridGlobal::getAlpha() const{ return alpha; }
double GridGlobal::getBeta() const{ return beta; }

int GridGlobal::getNumLoaded() const{ return (num_outputs == 0) ? 0 : points.getNumIndexes(); }
int GridGlobal::getNumNeeded() const{ return needed.getNumIndexes(); }
int GridGlobal::getNumPoints() const{ return ((points.empty()) ? needed.getNumIndexes() : points.getNumIndexes()); }

void GridGlobal::mapIndexesToNodes(const std::vector<int> *indexes, double *x) const{
    int num_points = (int) (indexes->size() / (size_t) num_dimensions);
    Data2D<double> splitx;
    splitx.load(num_dimensions, num_points, x);
    Data2D<int> spliti;
    spliti.cload(num_dimensions, num_points, indexes->data());
    #pragma omp parallel for schedule(static)
    for(int i=0; i<num_points; i++){
        const int *p = spliti.getCStrip(i);
        double *xx = splitx.getStrip(i);
        for(int j=0; j<num_dimensions; j++)
            xx[j] = wrapper.getNode(p[j]);
    }
}

void GridGlobal::getLoadedPoints(double *x) const{
    mapIndexesToNodes(points.getVector(), x);
}
void GridGlobal::getNeededPoints(double *x) const{
    mapIndexesToNodes(needed.getVector(), x);
}
void GridGlobal::getPoints(double *x) const{
    if (points.empty()){ getNeededPoints(x); }else{ getLoadedPoints(x); };
}

void GridGlobal::getQuadratureWeights(double weights[]) const{
    std::fill_n(weights, (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes(), 0.0);

    std::vector<int> num_oned_points(num_dimensions);
    for(int n=0; n<active_tensors.getNumIndexes(); n++){
        const int* levels = active_tensors.getIndex(n);
        num_oned_points[0] = wrapper.getNumPoints(levels[0]);
        int num_tensor_points = num_oned_points[0];
        for(int j=1; j<num_dimensions; j++){
            num_oned_points[j] = wrapper.getNumPoints(levels[j]);
            num_tensor_points *= num_oned_points[j];
        }
        double tensor_weight = (double) active_w[n];

        #pragma omp parallel for schedule(static)
        for(int i=0; i<num_tensor_points; i++){
            int t = i;
            double w = 1.0;
            for(int j=num_dimensions-1; j>=0; j--){
                w *= wrapper.getWeight(levels[j], t % num_oned_points[j]);
                t /= num_oned_points[j];
            }
            weights[tensor_refs[n][i]] += tensor_weight * w;
        }
    }
}

void GridGlobal::getInterpolationWeights(const double x[], double weights[]) const{
    std::fill_n(weights, (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes(), 0.0);

    CacheLagrange<double> lcache(num_dimensions, max_levels, &wrapper, x);

    std::vector<int> num_oned_points(num_dimensions);
    for(int n=0; n<active_tensors.getNumIndexes(); n++){
        const int* levels = active_tensors.getIndex(n);
        num_oned_points[0] = wrapper.getNumPoints(levels[0]);
        int num_tensor_points = num_oned_points[0];
        for(int j=1; j<num_dimensions; j++){
            num_oned_points[j] = wrapper.getNumPoints(levels[j]);
            num_tensor_points *= num_oned_points[j];
        }
        double tensor_weight = (double) active_w[n];
        for(int i=0; i<num_tensor_points; i++){
            int t = i;
            double w = 1.0;
            for(int j=num_dimensions-1; j>=0; j--){
                w *= lcache.getLagrange(j, levels[j], t % num_oned_points[j]);
                t /= num_oned_points[j];
            }
            weights[tensor_refs[n][i]] += tensor_weight * w;
        }
    }
}

void GridGlobal::acceptUpdatedTensors(){
    if (points.empty()){
        points = std::move(needed);
        needed = MultiIndexSet();
    }else if (!needed.empty()){
        points.addMultiIndexSet(needed);
        needed = MultiIndexSet();

        tensors = std::move(updated_tensors);
        updated_tensors = MultiIndexSet();

        active_tensors = std::move(updated_active_tensors);
        updated_active_tensors = MultiIndexSet();

        active_w = std::move(updated_active_w);
        updated_active_w = std::vector<int>();

        int m;
        MultiIndexManipulations::getMaxIndex(tensors, max_levels, m);

        recomputeTensorRefs(points);
    }
}

void GridGlobal::loadNeededPoints(const double *vals, TypeAcceleration){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_vals.clear();
    #endif
    if (points.empty() || needed.empty()){
        values.setValues(vals);
    }else{
        values.addValues(points, needed, vals);
    }
    acceptUpdatedTensors();
}
void GridGlobal::mergeRefinement(){
    if (needed.empty()) return; // nothing to do
    int num_all_points = getNumLoaded() + getNumNeeded();
    std::vector<double> vals(((size_t) num_all_points) * ((size_t) num_outputs), 0.0);
    values.setValues(vals);
    acceptUpdatedTensors();
}

void GridGlobal::beginConstruction(){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal(num_dimensions, num_outputs));
    if (points.empty()){ // if we start dynamic construction from an empty grid
        for(int i=0; i<tensors.getNumIndexes(); i++){
            const int *t = tensors.getIndex(i);
            double weight = -1.0 / (1.0 + (double) std::accumulate(t, t + num_dimensions, 0));
            dynamic_values->addTensor(t, [&](int l)->int{ return wrapper.getNumPoints(l); }, weight);
        }
        tensors = MultiIndexSet(num_dimensions);
        active_tensors = MultiIndexSet();
        active_w = std::vector<int>();
        needed = MultiIndexSet();
        values.resize(num_outputs, 0);
    }
}
void GridGlobal::writeConstructionDataBinary(std::ofstream &ofs) const{
    dynamic_values->writeBinary(ofs);
}
void GridGlobal::writeConstructionData(std::ofstream &ofs) const{
    dynamic_values->write(ofs);
}
void GridGlobal::readConstructionDataBinary(std::ifstream &ifs){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal(num_dimensions, num_outputs));
    int max_level = dynamic_values->readBinary(ifs);
    if (max_level + 1 > wrapper.getNumLevels())
        wrapper.load(custom, max_level, rule, alpha, beta);
    dynamic_values->reloadPoints([&](int l)->int{ return wrapper.getNumPoints(l); });
}
void GridGlobal::readConstructionData(std::ifstream &ifs){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>(new DynamicConstructorDataGlobal(num_dimensions, num_outputs));
    int max_level = dynamic_values->read(ifs);
    if (max_level + 1 > wrapper.getNumLevels())
        wrapper.load(custom, max_level, rule, alpha, beta);
    dynamic_values->reloadPoints([&](int l)->int{ return wrapper.getNumPoints(l); });
}
void GridGlobal::getCandidateConstructionPoints(TypeDepth type, int output, std::vector<double> &x, const std::vector<int> &level_limits){
    std::vector<int> weights;
    if ((type == type_iptotal) || (type == type_ipcurved) || (type == type_qptotal) || (type == type_qpcurved)){
        int min_needed_points = ((type == type_ipcurved) || (type == type_qpcurved)) ? 4 * num_dimensions : 2 * num_dimensions;
        if (points.getNumIndexes() > min_needed_points) // if there are enough points to estimate coefficients
            estimateAnisotropicCoefficients(type, output, weights);
    }
    getCandidateConstructionPoints(type, weights, x, level_limits);
}
void GridGlobal::getCandidateConstructionPoints(TypeDepth type, const std::vector<int> &weights, std::vector<double> &x, const std::vector<int> &level_limits){
    std::vector<int> proper_weights = weights;
    std::vector<double> curved_weights;
    double hyper_denom;
    TypeDepth contour_type = type;
    if ((type == type_hyperbolic) || (type == type_iphyperbolic) || (type == type_qphyperbolic)){
        contour_type = type_hyperbolic;
        if (proper_weights.empty()){
            curved_weights = std::vector<double>(num_dimensions, 1.0);
            hyper_denom = 1.0;
        }else{
            curved_weights.resize(num_dimensions);
            std::transform(proper_weights.begin(), proper_weights.end(), curved_weights.begin(), [&](int i)->double{ return (double) i; });
            hyper_denom = (double) std::accumulate(proper_weights.begin(), proper_weights.end(), 1);
        }
    }else if ((type == type_curved) || (type == type_ipcurved) || (type == type_qpcurved)){
        contour_type = type_curved;
        if (proper_weights.empty()){
            proper_weights = std::vector<int>(num_dimensions, 1);
            contour_type = type_level;
        }else{
            proper_weights.resize(num_dimensions);
            curved_weights = std::vector<double>(num_dimensions);
            auto itr = weights.begin() + num_dimensions;
            for(auto &w : curved_weights) w = (double) *itr++;
        }
    }else{
        contour_type = type_level;
        if (proper_weights.empty()) proper_weights = std::vector<int>(num_dimensions, 1);
    }

    std::vector<int> cached_exactness;

    getCandidateConstructionPoints([&](const int *t) -> double{
        // cache the exactness (interpolation/quadrature) or the level for the tensors
        // the lambda defined here is called after wrapper is updated, thus can use wrapper.getNumLevels()
        // the caching will be performed once
        if (cached_exactness.size() < (size_t) wrapper.getNumLevels()){
            cached_exactness.resize(wrapper.getNumLevels());
            if ((type == type_iptotal) || (type == type_ipcurved) || (type == type_iphyperbolic)){
                cached_exactness[0] = 0;
                if (rule == rule_customtabulated){
                    for(size_t i=1; i<cached_exactness.size(); i++) cached_exactness[i] = custom.getIExact((int) i - 1) + 1;
                }else{
                    for(size_t i=1; i<cached_exactness.size(); i++) cached_exactness[i] = OneDimensionalMeta::getIExact((int) i - 1, rule) + 1;
                }
            }else if ((type == type_qptotal) || (type == type_qpcurved) || (type == type_qphyperbolic)){
                cached_exactness[0] = 0;
                if (rule == rule_customtabulated){
                    for(size_t i=1; i<cached_exactness.size(); i++) cached_exactness[i] = custom.getQExact((int) i - 1) + 1;
                }else{
                    for(size_t i=1; i<cached_exactness.size(); i++) cached_exactness[i] = OneDimensionalMeta::getQExact((int) i - 1, rule) + 1;
                }
            }else{
                for(size_t i=0; i<cached_exactness.size(); i++) cached_exactness[i] = (int) i;
            }
        }

        // replace the tensor with the cached_exactness which correspond to interpolation/quadrature exactness or simple level
        std::vector<int> wt(num_dimensions);
        std::transform(t, t + num_dimensions, wt.begin(), [&](const int &i)->int{ return cached_exactness[i]; });

        if (contour_type == type_level){
            return (double) std::inner_product(wt.begin(), wt.end(), proper_weights.data(), 0);
        }else if (contour_type == type_hyperbolic){
            double result = 1.0;
            auto itr = curved_weights.begin();
            for(auto w : wt){
                result *= pow((double) (1.0 + w), *itr++ / hyper_denom);
            }
            return result;
        }else{
            double result = (double) std::inner_product(wt.begin(), wt.end(), proper_weights.data(), 0);
            auto itr = curved_weights.begin();
            for(auto w : wt){
                result += *itr++ * log1p((double) w);
            }
            return result;
        }
    }, x, level_limits);
}
void GridGlobal::getCandidateConstructionPoints(std::function<double(const int *)> getTensorWeight, std::vector<double> &x, const std::vector<int> &level_limits){
    dynamic_values->clearTesnors(); // clear old tensors
    MultiIndexSet init_tensors;
    dynamic_values->getInitialTensors(init_tensors); // get the initial tensors (created with make grid)

    MultiIndexSet new_tensors;
    if (level_limits.empty()){
        MultiIndexManipulations::addExclusiveChildren<false>(tensors, init_tensors, level_limits, new_tensors);
    }else{
        MultiIndexManipulations::addExclusiveChildren<true>(tensors, init_tensors, level_limits, new_tensors);
    }

    if (!new_tensors.empty()){
        int max_level;
        MultiIndexManipulations::getMaxIndex(new_tensors, max_levels, max_level);
        if (max_level+1 > wrapper.getNumLevels())
            wrapper.load(custom, max_level, rule, alpha, beta);
    }

    std::vector<double> tweights(new_tensors.getNumIndexes());
    for(int i=0; i<new_tensors.getNumIndexes(); i++)
        tweights[i] = (double) getTensorWeight(new_tensors.getIndex(i));

    for(int i=0; i<new_tensors.getNumIndexes(); i++){
        const int *t = new_tensors.getIndex(i);
        dynamic_values->addTensor(t, [&](int l)->int{ return wrapper.getNumPoints(l); }, tweights[i]);
    }
    std::vector<int> node_indexes;
    dynamic_values->getNodesIndexes(node_indexes);
    x.resize(node_indexes.size());
    mapIndexesToNodes(&node_indexes, x.data());
}
void GridGlobal::loadConstructedPoint(const double x[], const std::vector<double> &y){
    std::vector<int> p(num_dimensions);
    for(int j=0; j<num_dimensions; j++){
        int i = 0;
        while(fabs(wrapper.getNode(i) - x[j]) > TSG_NUM_TOL) i++; // convert canonical node to index
        p[j] = i;
    }

    if (dynamic_values->addNewNode(p, y)){ // if a new tensor is complete
        loadConstructedTensors();
    }
}
void GridGlobal::loadConstructedTensors(){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_vals.clear();
    #endif
    std::vector<int> tensor;
    MultiIndexSet new_points;
    std::vector<double> new_values;
    bool added_any = false;
    while(dynamic_values->ejectCompleteTensor(tensors, tensor, new_points, new_values)){
        if (points.empty()){
            values.setValues(new_values);
            points = std::move(new_points);
        }else{
            values.addValues(points, new_points, new_values.data());
            points.addMultiIndexSet(new_points);
        }

        tensors.addSortedInsexes(tensor);
        added_any = true;
    }

    if (added_any){
        std::vector<int> tensors_w;
        MultiIndexManipulations::computeTensorWeights(tensors, tensors_w);
        MultiIndexManipulations::createActiveTensors(tensors, tensors_w, active_tensors);

        active_w = std::vector<int>();
        active_w.reserve(active_tensors.getNumIndexes());
        for(auto w : tensors_w) if (w != 0) active_w.push_back(w);

        recomputeTensorRefs(points);
    }
}
void GridGlobal::finishConstruction(){
    dynamic_values = std::unique_ptr<DynamicConstructorDataGlobal>();
}

const double* GridGlobal::getLoadedValues() const{
    if (getNumLoaded() == 0) return 0;
    return values.getValues(0);
}

void GridGlobal::evaluate(const double x[], double y[]) const{
    std::vector<double> w(points.getNumIndexes());
    getInterpolationWeights(x, w.data());
    TasBLAS::setzero(num_outputs, y);
    for(int i=0; i<points.getNumIndexes(); i++){
        const double *v = values.getValues(i);
        double wi = w[i];
        for(int k=0; k<num_outputs; k++) y[k] += wi * v[k];
    }
}
void GridGlobal::evaluateBatch(const double x[], int num_x, double y[]) const{
    Data2D<double> xx; xx.cload(num_dimensions, num_x, x);
    Data2D<double> yy; yy.load(num_outputs, num_x, y);
    #pragma omp parallel for
    for(int i=0; i<num_x; i++)
        evaluate(xx.getCStrip(i), yy.getStrip(i));
}

#ifdef Tasmanian_ENABLE_BLAS
void GridGlobal::evaluateFastCPUblas(const double x[], double y[]) const{
    std::vector<double> w(points.getNumIndexes());
    getInterpolationWeights(x, w.data());
    TasBLAS::dgemv(num_outputs, points.getNumIndexes(), values.getValues(0), w.data(), y);
}
void GridGlobal::evaluateBatchCPUblas(const double x[], int num_x, double y[]) const{
    int num_points = points.getNumIndexes();
    Data2D<double> weights; weights.resize(num_points, num_x);
    evaluateHierarchicalFunctions(x, num_x, weights.getStrip(0));

    TasBLAS::dgemm(num_outputs, num_x, num_points, 1.0, values.getValues(0), weights.getStrip(0), 0.0, y);
}
#endif // Tasmanian_ENABLE_BLAS

#ifdef Tasmanian_ENABLE_CUDA
void GridGlobal::evaluateFastGPUcublas(const double x[], double y[]) const{
    if (cuda_vals.size() == 0) cuda_vals.load(*(values.aliasValues()));

    std::vector<double> weights(points.getNumIndexes());
    getInterpolationWeights(x, weights.data());

    cuda_engine.cublasDGEMM(num_outputs, 1, points.getNumIndexes(), 1.0, cuda_vals, weights, 0.0, y);
}
void GridGlobal::evaluateFastGPUcuda(const double x[], double y[]) const{
    evaluateFastGPUcublas(x, y);
}
void GridGlobal::evaluateBatchGPUcublas(const double x[], int num_x, double y[]) const{
    if (cuda_vals.size() == 0) cuda_vals.load(*(values.aliasValues()));

    int num_points = points.getNumIndexes();
    Data2D<double> weights; weights.resize(num_points, num_x);
    evaluateHierarchicalFunctions(x, num_x, weights.getStrip(0));

    cuda_engine.cublasDGEMM(num_outputs, num_x, num_points, 1.0, cuda_vals, *(weights.getVector()), 0.0, y);
}
void GridGlobal::evaluateBatchGPUcuda(const double x[], int num_x, double y[]) const{
    evaluateBatchGPUcublas(x, num_x, y);
}
#endif // Tasmanian_ENABLE_CUDA

#ifdef Tasmanian_ENABLE_MAGMA
void GridGlobal::evaluateFastGPUmagma(int gpuID, const double x[], double y[]) const{
    if (cuda_vals.size() == 0) cuda_vals.load(*(values.aliasValues()));

    std::vector<double> weights(points.getNumIndexes());
    getInterpolationWeights(x, weights.data());

    cuda_engine.magmaCudaDGEMM(gpuID, num_outputs, 1, points.getNumIndexes(), 1.0, cuda_vals, weights, 0.0, y);
}
void GridGlobal::evaluateBatchGPUmagma(int gpuID, const double x[], int num_x, double y[]) const{
    if (cuda_vals.size() == 0) cuda_vals.load(*(values.aliasValues()));

    int num_points = points.getNumIndexes();
    Data2D<double> weights; weights.resize(num_points, num_x);
    evaluateHierarchicalFunctions(x, num_x, weights.getStrip(0));

    cuda_engine.magmaCudaDGEMM(gpuID, num_outputs, num_x, num_points, 1.0, cuda_vals, *(weights.getVector()), 0.0, y);
}
#endif // Tasmanian_ENABLE_MAGMA

void GridGlobal::integrate(double q[], double *conformal_correction) const{
    std::vector<double> w(getNumPoints());
    getQuadratureWeights(w.data());
    if (conformal_correction != 0) for(int i=0; i<points.getNumIndexes(); i++) w[i] *= conformal_correction[i];
    std::fill(q, q+num_outputs, 0.0);
    #pragma omp parallel for schedule(static)
    for(int k=0; k<num_outputs; k++){
        for(int i=0; i<points.getNumIndexes(); i++){
            const double *v = values.getValues(i);
            q[k] += w[i] * v[k];
        }
    }
}

void GridGlobal::evaluateHierarchicalFunctions(const double x[], int num_x, double y[]) const{
    int num_points = (points.empty()) ? needed.getNumIndexes() : points.getNumIndexes();
    Data2D<double> yy; yy.load(num_points, num_x, y);
    Data2D<double> xx; xx.cload(num_dimensions, num_x, x);
    #pragma omp parallel for
    for(int i=0; i<num_x; i++){
        getInterpolationWeights(xx.getCStrip(i), yy.getStrip(i));
    }
}

void GridGlobal::computeSurpluses(int output, bool normalize, std::vector<double> &surp) const{
    int num_points = points.getNumIndexes();
    surp.resize(num_points);

    if (OneDimensionalMeta::isSequence(rule)){
        double max_surp = 0.0;
        for(int i=0; i<num_points; i++){
            surp[i] = values.getValues(i)[output];
            if (fabs(surp[i]) > max_surp) max_surp = fabs(surp[i]);
        }

        GridSequence seq; // there is an extra copy here, but the sequence grid does the surplus computation automatically
        MultiIndexSet spoints = points;
        seq.setPoints(spoints, 1, rule);

        seq.loadNeededPoints(surp.data());

        std::copy_n(seq.getSurpluses(), surp.size(), surp.data());

        if (normalize) for(auto &s : surp) s /= max_surp;
    }else{
        MultiIndexSet polynomial_set(num_dimensions);
        getPolynomialSpace(true, polynomial_set);

        MultiIndexSet quadrature_tensors(num_dimensions);
        MultiIndexManipulations::generateLowerMultiIndexSet<int>([&](const std::vector<int> &index) ->
            bool{
                std::vector<int> qindex = index;
                for(auto &i : qindex) i = (i > 0) ? 1 + OneDimensionalMeta::getQExact(i - 1, rule_gausspatterson) : 0;
                return polynomial_set.missing(qindex);
            }, quadrature_tensors);

        int getMaxQuadLevel = *std::max_element(quadrature_tensors.getVector()->begin(), quadrature_tensors.getVector()->end());

        GridGlobal QuadGrid;
        if (getMaxQuadLevel < TableGaussPatterson::getNumLevels()-1){
            QuadGrid.setTensors(quadrature_tensors, 0, rule_gausspatterson, 0.0, 0.0);
        }else{
            MultiIndexManipulations::generateLowerMultiIndexSet<int>([&](const std::vector<int> &index) ->
                bool{
                    std::vector<int> qindex = index;
                    for(auto &i : qindex) i = (i > 0) ? 1 + OneDimensionalMeta::getQExact(i - 1, rule_clenshawcurtis) : 0;
                    return polynomial_set.missing(qindex);
                }, quadrature_tensors);
            QuadGrid.setTensors(quadrature_tensors, 0, rule_clenshawcurtis, 0.0, 0.0);
        }

        size_t qn = (size_t) QuadGrid.getNumPoints();
        std::vector<double> w(qn);
        QuadGrid.getQuadratureWeights(w.data());
        std::vector<double> x(qn * ((size_t) num_dimensions));
        std::vector<double> y(qn * ((size_t) num_outputs));
        QuadGrid.getPoints(x.data());
        std::vector<double> I(qn);

        evaluateBatch(x.data(), (int) qn, y.data());
        auto iter_y = y.begin();
        std::advance(iter_y, output);
        for(auto &ii : I){
            ii = *iter_y;
            std::advance(iter_y, num_outputs);
        }

        #pragma omp parallel for schedule(dynamic)
        for(int i=0; i<num_points; i++){
            // for each surp, do the quadrature
            const int* p = points.getIndex(i);
            double c = 0.0;
            auto iter_x = x.begin();
            for(auto ii: I){
                double v = 1.0;
                for(int j=0; j<num_dimensions; j++) v *= legendre(p[j], *iter_x++);
                c += v * ii;
            }
            double nrm = sqrt((double) p[0] + 0.5);
            for(int j=1; j<num_dimensions; j++){
                nrm *= sqrt((double) p[j] + 0.5);
            }
            surp[i] = c * nrm;
        }
    }
}

void GridGlobal::estimateAnisotropicCoefficients(TypeDepth type, int output, std::vector<int> &weights) const{
    double tol = 1000.0 * TSG_NUM_TOL;
    std::vector<double> surp;
    computeSurpluses(output, false, surp);

    int num_points = points.getNumIndexes();

    int n = 0, m;
    for(int j=0; j<num_points; j++){
        surp[j] = fabs(surp[j]);
        if (surp[j] > tol) n++;
    }

    std::vector<double> b(n);
    Data2D<double> A;

    if ((type == type_curved) || (type == type_ipcurved) || (type == type_qpcurved)){
        m = 2*num_dimensions + 1;
        A.resize(n, m);

        int count = 0;
        for(int c=0; c<num_points; c++){
            const int *indx = points.getIndex(c);
            if (surp[c] > tol){
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(j)[count] = ((double) indx[j]);
                }
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(num_dimensions + j)[count] = log((double) (indx[j] + 1));
                }
                A.getStrip(2*num_dimensions)[count] = 1.0;
                b[count++] = -log(surp[c]);
            }
        }
    }else{
        m = num_dimensions + 1;
        A.resize(n, m);

        int count = 0;
        for(int c=0; c<num_points; c++){
            const int *indx = points.getIndex(c);
            if (surp[c] > tol){
                for(int j=0; j<num_dimensions; j++){
                    A.getStrip(j)[count] = ((double) indx[j]);
                }
                A.getStrip(num_dimensions)[count] = 1.0;
                b[count++] = - log(surp[c]);
            }
        }
    }

    std::vector<double> x(m);
    TasmanianDenseSolver::solveLeastSquares(n, m, A.getStrip(0), b.data(), 1.E-5, x.data());

    weights.resize(--m);
    for(int j=0; j<m; j++){
        weights[j] = (int)(x[j] * 1000.0 + 0.5);
    }

    int min_weight = weights[0];  for(int j=1; j<num_dimensions; j++){ if (min_weight < weights[j]) min_weight = weights[j];  } // start by finding the largest weight
    if (min_weight < 0){ // all directions are diverging, default to isotropic total degree
        for(int j=0; j<num_dimensions; j++)  weights[j] = 1;
        if (m == 2*num_dimensions) for(int j=num_dimensions; j<2*num_dimensions; j++)  weights[j] = 0;
    }else{
        for(int j=0; j<num_dimensions; j++)  if((weights[j]>0) && (weights[j]<min_weight)) min_weight = weights[j];  // find the smallest positive weight
        for(int j=0; j<num_dimensions; j++){
            if (weights[j] <= 0){
                weights[j] = min_weight;
                if (m == 2*num_dimensions){
                    if (std::abs(weights[num_dimensions + j]) > weights[j]){
                        weights[num_dimensions + j] = (weights[num_dimensions + j] > 0.0) ? weights[j] : -weights[j];
                    }
                }
            }
        }
    }
}

void GridGlobal::setAnisotropicRefinement(TypeDepth type, int min_growth, int output, const std::vector<int> &level_limits){
    clearRefinement();
    std::vector<int> weights;
    estimateAnisotropicCoefficients(type, output, weights);

    int level = 0;
    do{
        updateGrid(++level, type, weights, level_limits);
    }while(getNumNeeded() < min_growth);
}

void GridGlobal::setSurplusRefinement(double tolerance, int output, const std::vector<int> &level_limits){
    clearRefinement();
    std::vector<double> surp;
    computeSurpluses(output, true, surp);

    int n = points.getNumIndexes();
    std::vector<bool> flagged(n);

    for(int i=0; i<n; i++)
        flagged[i] = (fabs(surp[i]) > tolerance);

    MultiIndexSet kids;
    MultiIndexManipulations::selectFlaggedChildren(points, flagged, level_limits, kids);

    if (kids.getNumIndexes() > 0){
        kids.addMultiIndexSet(points);
        MultiIndexManipulations::completeSetToLower<int>(kids);

        updated_tensors = std::move(kids);
        proposeUpdatedTensors();
    }
}
void GridGlobal::setHierarchicalCoefficients(const double c[], TypeAcceleration acc){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_vals.clear();
    #endif
    if (!points.empty()) clearRefinement();
    loadNeededPoints(c, acc);
}

double GridGlobal::legendre(int n, double x){
    if (n == 0) return 1.0;
    if (n == 1) return x;

    double lm = 1, l = x;
    for(int i=2; i<=n; i++){
        double lp = ((2*i - 1) * x * l) / ((double) i) - ((i - 1) * lm) / ((double) i);
        lm = l; l = lp;
    }
    return l;
}

void GridGlobal::clearAccelerationData(){
    #ifdef Tasmanian_ENABLE_CUDA
    cuda_engine.reset();
    cuda_vals.clear();
    #endif
}

void GridGlobal::getPolynomialSpace(bool interpolation, MultiIndexSet &polynomial_set) const{
    if (interpolation){
        if (rule == rule_customtabulated){
            MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return custom.getIExact(l); }, polynomial_set);
        }else{
            MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return OneDimensionalMeta::getIExact(l, rule); }, polynomial_set);
        }
    }else{
        if (rule == rule_customtabulated){
            MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return custom.getQExact(l); }, polynomial_set);
        }else{
            MultiIndexManipulations::createPolynomialSpace(active_tensors, [&](int l)-> int{ return OneDimensionalMeta::getQExact(l, rule); }, polynomial_set);
        }
    }
}

void GridGlobal::getPolynomialSpace(bool interpolation, int &n, int* &poly) const{
    MultiIndexSet polynomial_set(num_dimensions);
    getPolynomialSpace(interpolation, polynomial_set);

    n = polynomial_set.getNumIndexes();
    poly = new int[polynomial_set.getVector()->size()];
    std::copy(polynomial_set.getVector()->begin(), polynomial_set.getVector()->end(), poly);
}
const int* GridGlobal::getPointIndexes() const{
    return ((points.empty()) ? needed.getIndex(0) : points.getIndex(0));
}

}

#endif
