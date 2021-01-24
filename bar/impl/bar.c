#include "cactus.h"
#include "sonLib.h"
#include "endAligner.h"
#include "poaBarAligner.h"
#include "flowerAligner.h"
#include "rescue.h"
#include "commonC.h"
#include "stCaf.h"
#include "stPinchGraphs.h"
#include "stPinchIterator.h"
#include "stateMachine.h"
#include "pairwiseAligner.h"

PairwiseAlignmentParameters *pairwiseAlignmentParameters_constructFromCactusParams(CactusParams *params) {
    PairwiseAlignmentParameters *p = pairwiseAlignmentBandingParameters_construct();
    p->gapGamma = cactusParams_get_float(params, 2, "bar", "gapGamma");
    p->splitMatrixBiggerThanThis = cactusParams_get_int(params, 2, "bar", "splitMatrixBiggerThanThis");
    p->anchorMatrixBiggerThanThis = cactusParams_get_int(params, 2, "bar", "anchorMatrixBiggerThanThis");
    p->repeatMaskMatrixBiggerThanThis = cactusParams_get_int(params, 2, "bar", "repeatMaskMatrixBiggerThanThis");
    p->diagonalExpansion = cactusParams_get_int(params, 2, "bar", "diagonalExpansion");
    p->constraintDiagonalTrim = cactusParams_get_int(params, 2, "bar", "constraintDiagonalTrim");
    p->alignAmbiguityCharacters = cactusParams_get_int(params, 2, "bar", "alignAmbiguityCharacters");
    return p;
}

stPinch *getNextAlignedPairAlignment(stSortedSetIterator *it) {
    AlignedPair *alignedPair = stSortedSet_getNext(it);
    if (alignedPair == NULL) {
        return NULL;
    }
    static stPinch pinch;
    stPinch_fillOut(&pinch, alignedPair->subsequenceIdentifier, alignedPair->reverse->subsequenceIdentifier, alignedPair->position,
                    alignedPair->reverse->position, 1, alignedPair->strand == alignedPair->reverse->strand);
    return &pinch;
}

static int64_t minimumIngroupDegree = 0, minimumOutgroupDegree = 0, minimumDegree = 0, minimumNumberOfSpecies = 0;
static Flower *flower;

bool blockFilterFn(stPinchBlock *pinchBlock) {
    return !stCaf_containsRequiredSpecies(pinchBlock, flower, minimumIngroupDegree, minimumOutgroupDegree, minimumDegree, minimumNumberOfSpecies);
}

void bar(stList *flowers, CactusParams *params, CactusDisk *cactusDisk, stList *listOfEndAlignmentFiles, bool cleanupMemory) {
    //////////////////////////////////////////////
    //Parse the many, many necessary parameters from the params file
    //////////////////////////////////////////////

    minimumIngroupDegree = cactusParams_get_int(params, 2, "bar", "minimumIngroupDegree");
    minimumOutgroupDegree = cactusParams_get_int(params, 2, "bar", "minimumOutgroupDegree");
    minimumDegree = cactusParams_get_int(params, 2, "bar", "minimumBlockDegree");
    minimumNumberOfSpecies = cactusParams_get_int(params, 2, "bar", "minimumNumberOfSpecies");

    // Hardcoded parameters
    int64_t chainLengthForBigFlower = 1000000;
    int64_t longChain = 2;
    int64_t maximumLength = 1500;

    int64_t spanningTrees = cactusParams_get_int(params, 2, "bar", "spanningTrees");
    bool useProgressiveMerging = cactusParams_get_int(params, 2, "bar", "useProgressiveMerging");
    float matchGamma = cactusParams_get_float(params, 2, "bar", "matchGamma");

    //bool useBanding = cactusParams_get_int(params, 2, "bar", "useBanding");
    //int64_t minimumSizeToRescue = cactusParams_get_int(params, 2, "bar", "minimumSizeToRescue");
    //double minimumCoverageToRescue = cactusParams_get_float(params, 2, "bar", "minimumCoverageToRescue");

    // toggle from pecan to abpoa for multiple alignment, by setting to non-zero
    // Note that poa uses about N^2 memory, so maximum value is generally in 10s of kb
    int64_t usePoa = cactusParams_get_int(params, 2, "bar", "partialOrderAlignment");
    int64_t poaWindow = cactusParams_get_int(params, 2, "bar", "partialOrderAlignmentWindow");
    int64_t maskFilter = cactusParams_get_int(params, 2, "bar", "partialOrderAlignmentMaskFilter");
    int64_t poaBandConstant = cactusParams_get_int(params, 2, "bar", "partialOrderAlignmentBandConstant");
    //defaults from abpoa
    double poaBandFraction = cactusParams_get_float(params, 2, "bar", "partialOrderAlignmentBandFraction");

    PairwiseAlignmentParameters *pairwiseAlignmentParameters = pairwiseAlignmentParameters_constructFromCactusParams(params);
    bool pruneOutStubAlignments = cactusParams_get_int(params, 2, "bar", "pruneOutStubAlignments");

    /*
     * Setup the input parameters for cactus core.
     */

    /*
     * Compute complete flower alignments, possibly loading some precomputed alignments.
     */
    /*bedRegion *bedRegions = NULL;
    size_t numBeds = 0;
    if (ingroupCoverageFilePath != NULL) {
        // Pre-load the mmap for the coverage file.
        FILE *coverageFile = fopen(ingroupCoverageFilePath, "rb");
        if (coverageFile == NULL) {
            st_errnoAbort("Opening coverage file %s failed",
                          ingroupCoverageFilePath);
        }
        fseek(coverageFile, 0, SEEK_END);
        int64_t coverageFileLen = ftell(coverageFile);
        assert(coverageFileLen >= 0);
        assert(coverageFileLen % sizeof(bedRegion) == 0);
        if (coverageFileLen == 0) {
            // mmap doesn't like length-0 mappings, for obvious
            // reasons. Pretend that the coverage file doesn't
            // exist in this case, since it contains no data.
            ingroupCoverageFilePath = NULL;
        } else {
            // Establish a memory mapping for the file.
            bedRegions = mmap(NULL, coverageFileLen, PROT_READ, MAP_SHARED,
                              fileno(coverageFile), 0);
            if (bedRegions == MAP_FAILED) {
                st_errnoAbort("Failure mapping coverage file");
            }

            numBeds = coverageFileLen / sizeof(bedRegion);
        }
        fclose(coverageFile);
    }*/

    //////////////////////////////////////////////
    //Run the bar algorithm
    //////////////////////////////////////////////

    if (listOfEndAlignmentFiles != NULL && stList_length(flowers) != 1) {
        st_errAbort("We have precomputed alignments but %" PRIi64 " flowers to align.\n", stList_length(flowers));
    }
    cactusDisk_preCacheStrings(cactusDisk, flowers);
    for (int64_t j = 0; j < stList_length(flowers); j++) {
        flower = stList_get(flowers, j);
        st_logInfo("Processing a flower\n");

        stPinchIterator *pinchIterator = NULL;
        stSortedSet *alignedPairs = NULL;
        stList *alignment_blocks = NULL;

        if(usePoa) {
            /*
             * This makes a consistent set of alignments using abPoa.
             *
             * It does not use any precomputed alignments, if they are provided they will be ignored
             */
            alignment_blocks = make_flower_alignment_poa(flower, maximumLength, poaWindow, maskFilter, poaBandConstant, poaBandFraction);
            st_logInfo("Created the poa alignments: %" PRIi64 " poa alignment blocks\n", stList_length(alignment_blocks));
            pinchIterator = stPinchIterator_constructFromAlignedBlocks(alignment_blocks);
        }
        else {
            StateMachine *sM = stateMachine5_construct(fiveState);
            alignedPairs = makeFlowerAlignment3(sM, flower, listOfEndAlignmentFiles, spanningTrees, maximumLength,
                                                useProgressiveMerging, matchGamma,
                                                pairwiseAlignmentParameters,
                                                pruneOutStubAlignments);
            stateMachine_destruct(sM);
            st_logInfo("Created the alignment: %" PRIi64 " pairs\n", stSortedSet_size(alignedPairs));
            pinchIterator = stPinchIterator_constructFromAlignedPairs(alignedPairs, getNextAlignedPairAlignment);
        }
        /*
         * Run the cactus caf functions to build cactus.
         */
        stPinchThreadSet *threadSet = stCaf_setup(flower);
        stCaf_anneal(threadSet, pinchIterator, NULL);
        if (minimumDegree < 2) {
            stCaf_makeDegreeOneBlocks(threadSet);
        }
        if (minimumIngroupDegree > 0 || minimumOutgroupDegree > 0 || minimumDegree > 1) {
            stCaf_melt(flower, threadSet, blockFilterFn, 0, 0, 0, INT64_MAX);
        }

        /*if (ingroupCoverageFilePath != NULL) {
            // Rescue any sequence that is covered by outgroups
            // but currently unaligned into single-degree blocks.
            stPinchThreadSetIt pinchIt = stPinchThreadSet_getIt(threadSet);
            stPinchThread *thread;
            while ((thread = stPinchThreadSetIt_getNext(&pinchIt)) != NULL) {
                Cap *cap = flower_getCap(flower,
                                         stPinchThread_getName(thread));
                assert(cap != NULL);
                Sequence *sequence = cap_getSequence(cap);
                assert(sequence != NULL);
                rescueCoveredRegions(thread, bedRegions, numBeds,
                                     sequence_getName(sequence),
                                     minimumSizeToRescue,
                                     minimumCoverageToRescue);
            }
            stCaf_joinTrivialBoundaries(threadSet);
        }*/

        stCaf_finish(flower, threadSet, chainLengthForBigFlower, longChain, INT64_MAX, INT64_MAX, cleanupMemory); //Flower now destroyed.
        stPinchThreadSet_destruct(threadSet);
        st_logInfo("Ran the cactus core script.\n");

        /*
         * Cleanup
         */
        //Clean up the sorted set after cleaning up the iterator
        stPinchIterator_destruct(pinchIterator);
        if(poaWindow != 0) {
            stList_destruct(alignment_blocks);
        }
        else {
            stSortedSet_destruct(alignedPairs);
        }

        st_logInfo("Finished filling in the alignments for the flower\n");
    }

    //////////////////////////////////////////////
    //Clean up
    //////////////////////////////////////////////

    stList_destruct(flowers);
    pairwiseAlignmentBandingParameters_destruct(pairwiseAlignmentParameters);

    /*if (bedRegions != NULL) {
        // Clean up our mapping.
        munmap(bedRegions, numBeds * sizeof(bedRegion));
    }*/
}
