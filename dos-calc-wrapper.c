#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <argp.h>
#include "dos-calc-velocity-decomposition.c"
#include "dos-calc-fft.c"

// uncomment next line for additional output
//#define DEBUG
#ifdef DEBUG
#define DPRINT(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( 0 )
#else
#define DPRINT(...)
#endif

// argp stuff
const char* argp_program_version = "dos-calc";
const char* argp_program_bug_address = "<bernhardt@cpc.tu-darmstadt.de>";
static char doc[] = "dos-calc -- a programm to calculate densities of states from trajectories";
static struct argp argp = { 0, 0, 0, doc };

// verbose print function
int verbosity = 0;

void verbPrintf(const char *format, ...)
{
    va_list args;

    if (!verbosity)
        return;

    va_start(args, format);
    vfprintf (stdout, format, args);
    va_end(args);
}

int main( int argc, char *argv[] )
{
    // command line stuff
    int dump_vel = 0;

    argp_parse(&argp, argc, argv, 0, 0, 0);

    // input
    char traj_file_name[400];
    int nblocks;
    long nblocksteps;
    long nfftsteps;
    int natoms;
    int nmols;
    int nmoltypes;

    FILE* f;
    FILE* fa;
    FILE* fb;
    FILE* fc;

    int result;
    int result2;

    fgets(traj_file_name, 399, stdin);
    // cut trailing newline of traj_file_name
    traj_file_name[strcspn(traj_file_name, "\r\n")] = 0;
    scanf("%d", &nblocks);
    scanf("%ld", &nblocksteps);

    nfftsteps = nblocksteps / 2 + 1;

    scanf("%d", &natoms);
    scanf("%d", &nmols);
    scanf("%d", &nmoltypes);

    DPRINT("Integers scanned:\n");
    DPRINT("%d\n", nblocks);
    DPRINT("%ld\n", nblocksteps);
    DPRINT("%d\n", natoms);
    DPRINT("%d\n", nmols);
    DPRINT("%d\n", nmoltypes);

    int* moltype_firstmol = calloc(nmoltypes, sizeof(int));
    for (int h=0; h<nmoltypes; h++)
    {
        scanf("%d", &moltype_firstmol[h]);
    }

    int* moltype_firstatom = calloc(nmoltypes, sizeof(int));
    for (int h=0; h<nmoltypes; h++)
    {
        scanf("%d", &moltype_firstatom[h]);
    }

    int* moltype_nmols = calloc(nmoltypes, sizeof(int));
    for (int h=0; h<nmoltypes; h++)
    {
        scanf("%d", &moltype_nmols[h]);
    }

    int* moltype_natomtypes = calloc(nmoltypes, sizeof(int));
    for (int h=0; h<nmoltypes; h++)
    {
        scanf("%d", &moltype_natomtypes[h]);
    }

    int* moltype_abc_indicators = calloc(nmoltypes * 4, sizeof(int));
    for (int h=0; h<nmoltypes; h++)
    {
        for (int i=0; i<4; i++)
        {
            scanf("%d", &moltype_abc_indicators[4*h + i]);
        }
    }

    int* mol_firstatom = calloc(nmols, sizeof(int));
    for (int i=0; i<nmols; i++)
    {
        scanf("%d", &mol_firstatom[i]);
    }

    int* mol_natoms = calloc(nmols, sizeof(int));
    for (int i=0; i<nmols; i++)
    {
        scanf("%d", &mol_natoms[i]);
    }

    float* mol_mass = calloc(nmols, sizeof(float));
    for (int i=0; i<nmols; i++)
    {
        scanf("%f", &mol_mass[i]);
    }

    int* mol_moltypenr = calloc(nmols, sizeof(int));
    for (int i=0; i<nmols; i++)
    {
        scanf("%d", &mol_moltypenr[i]);
    }

    float* atom_mass = calloc(natoms, sizeof(float));
    for (int j=0; j<natoms; j++)
    {
        scanf("%f", &atom_mass[j]);
    }

    // output
    float* mol_moments_of_inertia = calloc(nmols*3, sizeof(float));
    float* moltype_dos_raw_trn = calloc(nmoltypes*nfftsteps, sizeof(float));
    float* moltype_dos_raw_rot = calloc(nmoltypes*nfftsteps, sizeof(float));
    float* moltype_dos_raw_rot_a = calloc(nmoltypes*nfftsteps, sizeof(float));
    float* moltype_dos_raw_rot_b = calloc(nmoltypes*nfftsteps, sizeof(float));
    float* moltype_dos_raw_rot_c = calloc(nmoltypes*nfftsteps, sizeof(float));
    float* moltype_dos_raw_vib = calloc(nmoltypes*nfftsteps, sizeof(float));

    verbPrintf("going through %d blocks\n", nblocks);
    for (int block=0; block<nblocks; block++)
    {
        verbPrintf("now doing block %d\n", block);


        DPRINT("start decomposition\n");

        float* mol_velocities_trn = calloc(nmols*3*nblocksteps, sizeof(float));
        float* omegas_sqrt_i = calloc(nmols*3*nblocksteps, sizeof(float));
        float* velocities_vib = calloc(natoms*3*nblocksteps, sizeof(float));

        result = decomposeVelocities (traj_file_name,
                nblocksteps,
                natoms, 
                nmols, 
                nmoltypes,
                mol_firstatom,
                mol_natoms,
                mol_moltypenr,
                atom_mass,
                mol_mass,
                moltype_natomtypes,
                moltype_abc_indicators,
                mol_velocities_trn,
                omegas_sqrt_i,
                velocities_vib,
                mol_moments_of_inertia);

        // dump first block only!
        if (dump_vel == 1 && block == 0)
        {
            DPRINT("start dumping (first block only)\n");
            f = fopen("mol_omega_sqrt_i.txt", "w");
            for (int h=0; h<nmoltypes; h++)
            {
                int first_dof = moltype_firstmol[h]*3;
                int last_dof = moltype_firstmol[h]*3 + moltype_nmols[h]*3;
                for (int i=first_dof; i<last_dof; i++)
                {
                    for (int t=0; t<nblocksteps; t++)
                    {
                        if (t!=0) fprintf(f, " ");
                        fprintf(f, "%f", omegas_sqrt_i[i*nblocksteps + t]);
                    }
                    fprintf(f, "\n");
                }
            }
            fclose(f);

            f = fopen("mol_velocities.txt", "w");
            for (int h=0; h<nmoltypes; h++)
            {
                int first_dof = moltype_firstmol[h]*3;
                int last_dof = moltype_firstmol[h]*3 + moltype_nmols[h]*3;
                for (int i=first_dof; i<last_dof; i++)
                {
                    for (int t=0; t<nblocksteps; t++)
                    {
                        if (t!=0) fprintf(f, " ");
                        fprintf(f, "%f", mol_velocities_trn[i*nblocksteps + t]);
                    }
                    fprintf(f, "\n");
                }
            }
            fclose(f);

            f = fopen("atom_velocities_vib.txt", "w");
            for (int h=0; h<nmoltypes; h++)
            {
                int first_dof_vib = moltype_firstatom[h]*3;
                int last_dof_vib = moltype_firstatom[h]*3 + moltype_nmols[h]*moltype_natomtypes[h]*3;
                VPRINT("%d: %d %d\n", h, first_dof_vib, last_dof_vib);
                for (int i=first_dof_vib; i<last_dof_vib; i++)
                {
                    for (int t=0; t<nblocksteps; t++)
                    {
                        if (t!=0) fprintf(f, " ");
                        VPRINT(f, "%f", velocities_vib[i*nblocksteps + t]);
                    }
                    fprintf(f, "\n");
                }
            }
            fclose(f);
        }

        result2 = DOSCalculation (nmoltypes,
                nblocksteps,
                nfftsteps,
                moltype_firstmol,
                moltype_firstatom,
                moltype_nmols,
                moltype_natomtypes,
                atom_mass,
                mol_velocities_trn,
                omegas_sqrt_i,
                velocities_vib,
                moltype_dos_raw_trn,
                moltype_dos_raw_rot,
                moltype_dos_raw_rot_a,
                moltype_dos_raw_rot_b,
                moltype_dos_raw_rot_c,
                moltype_dos_raw_vib);

    } 
    DPRINT("finished all blocks\n");

    f = fopen("moltype_dos_raw_trn.txt", "w");
    for (int h=0; h<nmoltypes; h++)
    {
        for (int t=0; t<nfftsteps; t++)
        {
            if (t!=0) fprintf(f, " ");
            fprintf(f, "%f", moltype_dos_raw_trn[h*nfftsteps + t]);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    f = fopen("moltype_dos_raw_rot.txt", "w");
    for (int h=0; h<nmoltypes; h++)
    {
        for (int t=0; t<nfftsteps; t++)
        {
            if (t!=0) fprintf(f, " ");
            fprintf(f, "%f", moltype_dos_raw_rot[h*nfftsteps + t]);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    // test dos_rot_abc
    fa = fopen("moltype_dos_raw_rot_a.txt", "w");
    fb = fopen("moltype_dos_raw_rot_b.txt", "w");
    fc = fopen("moltype_dos_raw_rot_c.txt", "w");
    for (int h=0; h<nmoltypes; h++)
    {
        for (int t=0; t<nfftsteps; t++)
        {
            if (t!=0) fprintf(fa, " ");
            if (t!=0) fprintf(fb, " ");
            if (t!=0) fprintf(fc, " ");
            fprintf(fa, "%f", moltype_dos_raw_rot_a[h*nfftsteps + t]);
            fprintf(fb, "%f", moltype_dos_raw_rot_b[h*nfftsteps + t]);
            fprintf(fc, "%f", moltype_dos_raw_rot_c[h*nfftsteps + t]);
        }
        fprintf(fa, "\n");
        fprintf(fb, "\n");
        fprintf(fc, "\n");
    }
    fclose(fa);
    fclose(fb);
    fclose(fc);

    f = fopen("moltype_dos_raw_vib.txt", "w");
    for (int h=0; h<nmoltypes; h++)
    {
        for (int t=0; t<nfftsteps; t++)
        {
            if (t!=0) fprintf(f, " ");
            fprintf(f, "%f", moltype_dos_raw_vib[h*nfftsteps + t]);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    f = fopen("mol_moments_of_inertia.txt", "w");
    for (int i=0; i<nmols; i++)
    {
        fprintf(f, "%f %f %f\n", mol_moments_of_inertia[3*i+0], 
                mol_moments_of_inertia[3*i+1], mol_moments_of_inertia[3*i+2]);
    }
    fclose(f);

    return result + result2;
}
