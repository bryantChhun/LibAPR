//////////////////////////////////////////////////////
///
/// Bevan Cheeseman 2018
///
/// Examples of iteration and access to particle neighbours on the face of the Particle Cells.
///
/// Usage:
///
/// (using output of Example_get_apr (hdf5 apr file)
///
/// Example_neigh -i input_image_tiff -d input_directory
///
/////////////////////////////////////////////////////

#include <algorithm>
#include <iostream>

#include "Example_neigh.hpp"

bool command_option_exists(char **begin, char **end, const std::string &option)
{
    return std::find(begin, end, option) != end;
}

char* get_command_option(char **begin, char **end, const std::string &option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

cmdLineOptions read_command_line_options(int argc, char **argv){

    cmdLineOptions result;

    if(argc == 1) {
        std::cerr << "Usage: \"Example_neigh -i input_apr_file -d directory\"" << std::endl;
        exit(1);
    }

    if(command_option_exists(argv, argv + argc, "-i"))
    {
        result.input = std::string(get_command_option(argv, argv + argc, "-i"));
    } else {
        std::cout << "Input file required" << std::endl;
        exit(2);
    }

    if(command_option_exists(argv, argv + argc, "-d"))
    {
        result.directory = std::string(get_command_option(argv, argv + argc, "-d"));
    }

    if(command_option_exists(argv, argv + argc, "-o"))
    {
        result.output = std::string(get_command_option(argv, argv + argc, "-o"));
    }

    return result;

}

int main(int argc, char **argv) {

    // INPUT PARSING

    cmdLineOptions options = read_command_line_options(argc, argv);

    // Filename
    std::string file_name = options.directory + options.input;

    // Read the apr file into the part cell structure
    Part_timer timer;

    timer.verbose_flag = true;

    // APR datastructure
    APR<uint16_t> apr;

    //read file
    apr.read_apr(file_name);

    ///////////////////////////
    ///
    /// Serial Neighbour Iteration (Only Von Neumann (Face) neighbours)
    ///
    /////////////////////////////////

    ExtraPartCellData<uint16_t> neigh_avg(apr);

    APR_iterator<uint16_t> neighbour_iterator(apr);

    timer.start_timer("APR serial iterator neighbours loop");

    //Basic serial iteration over all particles
    for (apr.begin(); apr.end() != 0; apr.it_forward()) {

        //now we only update the neighbours, and directly access them through a neighbour iterator
        apr.update_all_neighbours();

        float counter = 0;
        float temp = 0;

        //loop over all the neighbours and set the neighbour iterator to it
        for (int dir = 0; dir < 6; ++dir) {
            // Neighbour Particle Cell Face definitions [+y,-y,+x,-x,+z,-z] =  [0,1,2,3,4,5]


            for (int index = 0; index < apr.number_neighbours_in_direction(dir); ++index) {
                // on each face, there can be 0-4 neighbours accessed by index
                if(neighbour_iterator.set_neighbour_iterator(apr, dir, index)){
                    //will return true if there is a neighbour defined

                    temp += neighbour_iterator(apr.particles_int);
                    counter++;

                }
            }
        }

        apr(neigh_avg) = temp/counter;

    }

    timer.stop_timer();

    ////////////////////////////
    ///
    /// OpenMP Parallel loop iteration (requires seperate iterators from the apr structure used in the serial examples above
    /// however, the functionality and access is exactly the same).
    ///
    ///////////////////////////

    //initialization of the iteration structures
    APR_iterator<uint16_t> apr_parallel_iterator(apr); //this is required for parallel access
    uint64_t part; //declare parallel iteration variable

    ExtraPartCellData<float> neigh_xm(apr);

    timer.start_timer("APR parallel iterator neighbour loop");

#pragma omp parallel for schedule(static) private(part) firstprivate(apr_parallel_iterator,neighbour_iterator)
    for (part = 0; part < apr.num_parts_total; ++part) {
        //needed step for any parallel loop (update to the next part)

        apr_parallel_iterator.set_iterator_to_particle_by_number(part);

        //compute neighbours as previously, now using the apr_parallel_iterator (APR_iterator), instead of the apr class for access.
        apr_parallel_iterator.update_all_neighbours();

        //loop over all the neighbours and set the neighbour iterator to it
        for (int dir = 0; dir < 6; ++dir) {
            for (int index = 0; index < apr_parallel_iterator.number_neighbours_in_direction(dir); ++index) {

                if(neighbour_iterator.set_neighbour_iterator(apr_parallel_iterator, dir, index)){
                    //neighbour_iterator works just like apr, and apr_parallel_iterator (you could also call neighbours)
                    apr_parallel_iterator(neigh_xm) += neighbour_iterator(apr.particles_int)*(apr_parallel_iterator.y() - neighbour_iterator.y());
                }

            }
        }

    }

    timer.stop_timer();


    /*
     *  Access only one directions neighbour
     */

    ExtraPartCellData<float> type_sum(apr);

    //need to initialize the neighbour iterator with the APR you are iterating over.

    timer.start_timer("APR parallel iterator neighbours loop only -x face neighbours");

#pragma omp parallel for schedule(static) private(part) firstprivate(apr_parallel_iterator,neighbour_iterator)
    for (part = 0; part < apr.num_parts_total; ++part) {
        //needed step for any parallel loop (update to the next part)
        apr_parallel_iterator.set_iterator_to_particle_by_number(part);

        const unsigned int dir = 3;
        //now we only update the neighbours, and directly access them through a neighbour iterator
        apr_parallel_iterator.update_direction_neighbours(dir); // 3 = -x face

        for (int index = 0; index < apr_parallel_iterator.number_neighbours_in_direction(dir); ++index) {
            // from 0 to 4 neighbours
            if(neighbour_iterator.set_neighbour_iterator(apr_parallel_iterator, dir, index)){
                //access data and perform a conditional sum (neighbour_iterator has all access like the normal iterator)
                if((neighbour_iterator.type() == 1) & (neighbour_iterator.depth() <= neighbour_iterator.depth_max())){
                    apr_parallel_iterator(type_sum) += neighbour_iterator(apr.particles_int)*apr_parallel_iterator.type();
                }
            }
        }
    }

    timer.stop_timer();

}


