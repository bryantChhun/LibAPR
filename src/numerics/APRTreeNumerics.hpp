//
// Created by cheesema on 15.02.18.
//

#ifndef LIBAPR_APRTREENUMERICS_HPP
#define LIBAPR_APRTREENUMERICS_HPP

#include "data_structures/APR/APRTree.hpp"
#include "data_structures/APR/APRTreeIterator.hpp"
#include "APRNumerics.hpp"

class APRTreeNumerics {

public:

    template<typename T, typename S, typename U>
    static void fill_tree_mean(APR<T> &apr, APRTree<T> &apr_tree, ExtraParticleData<S> &particle_data,ExtraParticleData<U> &tree_data) {

        APRTimer timer;
        timer.verbose_flag = true;

        timer.start_timer("ds-init");
        tree_data.init(apr_tree.total_number_parent_cells());

        //std::fill(tree_data.data.begin(), tree_data.data.end(), 0);

        APRTreeIterator treeIterator = apr_tree.tree_iterator();
        APRTreeIterator parentIterator = apr_tree.tree_iterator();

        APRIterator apr_iterator = apr.iterator();

        int z_d;
        int x_d;
        timer.stop_timer();
        timer.start_timer("ds-1l");

        for (unsigned int level = apr_iterator.level_max(); level >= apr_iterator.level_min(); --level) {
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic) private(x_d, z_d) firstprivate(apr_iterator, parentIterator)
#endif
            for (z_d = 0; z_d < apr.spatial_index_z_max(level) / 2; z_d++) {
                for (int z = 2 * z_d; z <= std::min(2 * z_d + 1, (int) apr.spatial_index_z_max(level)); ++z) {
                    //the loop is bundled into blocks of 2, this prevents race conditions with OpenMP parents
                    for (x_d = 0; x_d < apr.spatial_index_x_max(level) / 2; ++x_d) {
                        for (int x = 2 * x_d; x <= std::min(2 * x_d + 1, (int) apr.spatial_index_x_max(level)); ++x) {

                            parentIterator.set_new_lzx(level - 1, z / 2, x / 2);

                            //dealing with boundary conditions
                            float scale_factor_xz =
                                    (((2 * apr.spatial_index_x_max(level - 1) != apr.spatial_index_x_max(level)) &&
                                      ((x / 2) == (apr.spatial_index_x_max(level - 1) - 1))) +
                                     ((2 * apr.spatial_index_z_max(level - 1) != apr.spatial_index_z_max(level)) &&
                                      (z / 2) == (apr.spatial_index_z_max(level - 1) - 1))) * 2;

                            if (scale_factor_xz == 0) {
                                scale_factor_xz = 1;
                            }

                            float scale_factor_yxz = scale_factor_xz;

                            if ((2 * apr.spatial_index_y_max(level - 1) != apr.spatial_index_y_max(level))) {
                                scale_factor_yxz = scale_factor_xz * 2;
                            }


                            for (apr_iterator.set_new_lzx(level, z, x);
                                 apr_iterator.global_index() <
                                 apr_iterator.end_index; apr_iterator.set_iterator_to_particle_next_particle()) {

                                while (parentIterator.y() != (apr_iterator.y() / 2)) {
                                    parentIterator.set_iterator_to_particle_next_particle();
                                }

                                if (parentIterator.y() == (apr.spatial_index_y_max(level - 1) - 1)) {
                                    tree_data[parentIterator] =
                                            scale_factor_yxz * apr.particles_intensities[apr_iterator] / 8.0f +
                                            tree_data[parentIterator];
                                } else {

                                    tree_data[parentIterator] =
                                            scale_factor_xz * apr.particles_intensities[apr_iterator] / 8.0f +
                                            tree_data[parentIterator];
                                }

                            }
                        }
                    }
                }
            }
        }

        timer.stop_timer();
        timer.start_timer("ds-2l");

        //then do the rest of the tree where order matters
        for (unsigned int level = treeIterator.level_max(); level > treeIterator.level_min(); --level) {
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic) private(x_d, z_d) firstprivate(treeIterator, parentIterator)
#endif
            for (z_d = 0; z_d < apr.spatial_index_z_max(level) / 2; z_d++) {
                for (int z = 2 * z_d; z <= std::min(2 * z_d + 1, (int) apr.spatial_index_z_max(level)); ++z) {
                    //the loop is bundled into blocks of 2, this prevents race conditions with OpenMP parents
                    for (x_d = 0; x_d < apr.spatial_index_x_max(level) / 2; ++x_d) {
                        for (int x = 2 * x_d; x <= std::min(2 * x_d + 1, (int) apr.spatial_index_x_max(level)); ++x) {

                            parentIterator.set_new_lzx(level - 1, z / 2, x / 2);

                            float scale_factor_xz =
                                    (((2 * apr.spatial_index_x_max(level - 1) != apr.spatial_index_x_max(level)) &&
                                      ((x / 2) == (apr.spatial_index_x_max(level - 1) - 1))) +
                                     ((2 * apr.spatial_index_z_max(level - 1) != apr.spatial_index_z_max(level)) &&
                                      ((z / 2) == (apr.spatial_index_z_max(level - 1) - 1)))) * 2;

                            if (scale_factor_xz == 0) {
                                scale_factor_xz = 1;
                            }

                            float scale_factor_yxz = scale_factor_xz;

                            if ((2 * apr.spatial_index_y_max(level - 1) != apr.spatial_index_y_max(level))) {
                                scale_factor_yxz = scale_factor_xz * 2;
                            }

                            for (treeIterator.set_new_lzx(level, z, x);
                                 treeIterator.global_index() <
                                 treeIterator.end_index; treeIterator.set_iterator_to_particle_next_particle()) {

                                while (parentIterator.y() != treeIterator.y() / 2) {
                                    parentIterator.set_iterator_to_particle_next_particle();
                                }

                                if (parentIterator.y() == (apr.spatial_index_y_max(level - 1) - 1)) {
                                    tree_data[parentIterator] = scale_factor_yxz * tree_data[treeIterator] / 8.0f +
                                                                tree_data[parentIterator];
                                } else {
                                    tree_data[parentIterator] = scale_factor_xz * tree_data[treeIterator] / 8.0f +
                                                                tree_data[parentIterator];
                                }

                            }
                        }
                    }
                }
            }
        }
        timer.stop_timer();
    }
};

#endif