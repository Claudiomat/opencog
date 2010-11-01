/** feature_selection.h --- 
 *
 * Copyright (C) 2010 Nil Geisweiller
 *
 * Author: Nil Geisweiller <nilg@laptop>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef _OPENCOG_FEATURE_SELECTION_H
#define _OPENCOG_FEATURE_SELECTION_H

#include <functional>

#include <opencog/util/numeric.h>
#include <opencog/util/foreach.h>
#include <opencog/util/lru_cache.h>

namespace opencog {

/**
 * Returns a set S of features following the algo:
 * 1.a) Select all relevant features (that score above 'threshold'), called 'rel'
 * 1.b) Select all redundant features among 'rel', called 'red'
 * 1.c) res += res - red
 * 2) remove 'rel' from the initial set 'features', called 'tf'
 * 3.a) select all pairs of relevant features from 'ft', called 'rel'
 * 3.b) select all redundant features among 'rel', called 'red'
 * 4) follow the same pattern with triplets, etc, until max_size.
 * 5) return 'res'
 *
 * @param features    The initial set of features to be selected from
 * @param scorer      The function to score a set of features
 * @param threshold   The threshold to select a set of feature
 * @param max_size    The maximum size of each feature set tested in the scorer
 * @param remove_red  Flag indicating whether redundant features are discarded
 *
 * @return            The set of selected features
 */
template<typename Scorer, typename FeatureSet>
FeatureSet incremental_selection(const FeatureSet& features, const Scorer& scorer,
                                 double threshold, unsigned int max_size = 1,
                                 bool remove_red = false) {
    lru_cache<Scorer> scorer_cache(std::pow((double)features.size(), (int)max_size),
                                   scorer);

    FeatureSet rel; // set of relevant features for a given iteration
    FeatureSet res; // set of relevant non-redundant features to return

    for(unsigned int i = 1; i <= max_size; i++) {
        // define the set of set of features to test for relevancy
        FeatureSet tf;
        std::set_difference(features.begin(), features.end(),
                            rel.begin(), rel.end(), std::inserter(tf, tf.begin()));
        std::set<FeatureSet> fss = powerset(tf, i, true);
        // add the set of relevant features for that iteration in rel
        rel.empty();
        foreach(const FeatureSet& fs, fss)
            if(scorer_cache(fs) > threshold)
                rel.insert(fs.begin(), fs.end());
        if(remove_red) {
            // define the set of set of features to test redundancy
            std::set<FeatureSet> nrfss = powerset(rel, i+1, true);
            // determine the set of redundant features
            FeatureSet red;
            foreach(const FeatureSet& fs, nrfss) {
                if(has_empty_intersection(fs, red)) {
                    foreach(const typename FeatureSet::value_type& f, fs) {
                        FeatureSet fs_without_f(fs);
                        fs_without_f.erase(f);
                        double score_diff = 
                            scorer_cache(fs) - scorer_cache(fs_without_f);
                        if(score_diff < threshold) {
                            red.insert(f);
                            break;
                        }
                    }
                }
            }
            // add in res the relevant non-redundant features
            std::set_difference(rel.begin(), rel.end(), red.begin(), red.end(),
                                std::inserter(res, res.begin()));
        } else {
            res.insert(rel.begin(), rel.end());
        }
    }
    return res;
}

} // ~namespace opencog

#endif // _OPENCOG_FEATURE_SELECTION_H