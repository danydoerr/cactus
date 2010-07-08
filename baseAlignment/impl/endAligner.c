#include "endAligner.h"
#include "multipleAligner.h"
#include "adjacencySequences.h"

AlignedPair *alignedPair_construct(Name sequence1, int32_t position1, bool strand1,
                                   Name sequence2, int32_t position2, bool strand2, int32_t score) {
    AlignedPair *alignedPair = st_malloc(sizeof(AlignedPair));
    alignedPair->sequence = sequence1;
    alignedPair->position = position1;
    alignedPair->strand = strand1;

    alignedPair->reverse = st_malloc(sizeof(AlignedPair));
    alignedPair->reverse->reverse = alignedPair;

    alignedPair->reverse->sequence = sequence2;
    alignedPair->reverse->position = position2;
    alignedPair->reverse->strand = strand2;

    alignedPair->score = score;
    alignedPair->reverse->score = score;

    return alignedPair;
}

void alignedPair_destruct(AlignedPair *alignedPair) {
    free(alignedPair); //We assume the reverse will be free independently.
}

static int alignedPair_cmpFnP(const AlignedPair *alignedPair1, const AlignedPair *alignedPair2) {
    int i = netMisc_nameCompare(alignedPair1->sequence, alignedPair2->sequence);
    if(i == 0) {
        i = alignedPair1->position - alignedPair2->position;
        if(i == 0) {
            i = alignedPair1->strand - alignedPair2->strand;
        }
    }
    return i;
}

int alignedPair_cmpFn(const AlignedPair *alignedPair1, const AlignedPair *alignedPair2) {
    int i = alignedPair_cmpFnP(alignedPair1, alignedPair2);
    if(i == 0) {
        i = alignedPair_cmpFnP(alignedPair1->reverse, alignedPair2->reverse);
    }
    return i;
}

stSortedSet *makeEndAlignment(End *end, int32_t spanningTrees, int32_t maxSequenceLength,
        void *modelParameters) {
    //Make an alignment of the sequences in the ends

    //Get the adjacency sequences to be aligned.
    Cap *cap;
    End_InstanceIterator *it = end_getInstanceIterator(end);
    stList *sequences = stList_construct3(0, (void (*)(void *))adjacencySequence_destruct);
    stList *strings = stList_construct();
    while((cap = end_getNext(it)) != NULL) {
        if(cap_getSide(cap)) {
            cap = cap_getReverse(cap);
        }
        AdjacencySequence *adjacencySequence = adjacencySequence_construct(cap, maxSequenceLength);
        //st_uglyf("I got the following %s %i %i %i \n", netMisc_nameToStringStatic(adjacencySequence->sequenceName), adjacencySequence->start, adjacencySequence->length, adjacencySequence->strand);
        stList_append(sequences, adjacencySequence);
        stList_append(strings, adjacencySequence->string);
    }
    end_destructInstanceIterator(it);

    //Convert the alignment pairs to an alignment of the caps..
    stList *alignment = makeAlignment(strings, spanningTrees, modelParameters);
    stSortedSet *sortedAlignment =
            stSortedSet_construct3((int (*)(const void *, const void *))alignedPair_cmpFn,
            (void (*)(void *))alignedPair_destruct);

    while(stList_length(alignment) > 0) {
        stIntTuple *alignedPair = stList_pop(alignment);
        assert(stIntTuple_length(alignedPair) == 5);
        AdjacencySequence *i = stList_get(sequences, stIntTuple_getPosition(alignedPair, 1));
        AdjacencySequence *j = stList_get(sequences, stIntTuple_getPosition(alignedPair, 3));
        int32_t offset1 = stIntTuple_getPosition(alignedPair, 2);
        int32_t offset2 = stIntTuple_getPosition(alignedPair, 4);
        AlignedPair *alignedPair2 = alignedPair_construct(
                i->sequenceName, i->start + (i->strand ? offset1 : -offset1), i->strand,
                j->sequenceName, j->start + (j->strand ? offset2 : -offset2), j->strand,
                stIntTuple_getPosition(alignedPair, 0));
        assert(stSortedSet_search(sortedAlignment, alignedPair2) == NULL);
        assert(stSortedSet_search(sortedAlignment, alignedPair2->reverse) == NULL);
        stSortedSet_insert(sortedAlignment, alignedPair2);
        stSortedSet_insert(sortedAlignment, alignedPair2->reverse);
    }

    //Cleanup
    stList_destruct(strings);
    stList_destruct(sequences);
    stList_destruct(alignment);

    return sortedAlignment;
}