#ifndef CACTUS_PSEUDO_CHROMOSOME_H_
#define CACTUS_PSEUDO_CHROMOSOME_H_

#include "cactusGlobals.h"

/*
 * Constructs a pseudo-chromosome. See reference_construct for a
 * description of this structure.
 */
PseudoChromosome *pseudoChromosome_construct(Reference *reference,
		End *_5End, End *_3End);

/*
 * Get the the pseudo-chromosome's name.
 */
Name pseudoChromosome_getName(PseudoChromosome *pseudoChromosome);

/*
 * Gets the 5' prime end of the pseudo-chromosome.
 */
End *pseudoChromosome_get5End(PseudoChromosome *pseudoChromosome);

/*
 * Gets the 3' prime end of the pseudo-chromosome.
 */
End *pseudoChromosome_get3End(PseudoChromosome *pseudoChromosome);

/*
 * Gets the reference that contains the pseudo-chromosome.
 */
Reference *pseudoChromosome_getReference(PseudoChromosome *pseudoChromosome);

/*
 * Returns the number of pseudo-adjacencies in the pseudo-chromosome.
 */
int32_t pseudoChromosome_getPseudoAdjacencyNumber(PseudoChromosome *pseudoChromosome);

/*
 * Get the first pseudo adjacency in the list.
 */
PseudoAdjacency *pseudoChromosome_getFirst(PseudoChromosome *pseudoChromosome);

/*
 * Get the last pseudo adjacency in the list.
 */
PseudoAdjacency *pseudoChromosome_getLast(PseudoChromosome *pseudoChromosome);

/*
 * Get pseudo-adjacency with the given name.
 */
PseudoAdjacency *pseudoChromosome_getPseudoAdjacency(PseudoChromosome *pseudoChromosome, Name name);

/*
 * Gets an iterator to iterate through the pseudo-adjacencies in the pseudo-chromosomes.
 */
PseudoChromsome_PseudoAdjacencyIterator *pseudoChromosome_getPseudoAdjacencyIterator(PseudoChromosome *pseudoChromosome);

/*
 * Gets the next pseudo-adjacency from the iterator.
 */
PseudoAdjacency *pseudoChromosome_getNextPseudoAdjacency(PseudoChromsome_PseudoAdjacencyIterator *pseudoAdjacencyIterator);

/*
 * Gets the previous pseudo-adjacency from the iterator.
 */
PseudoAdjacency *pseudoChromosome_getPreviousPseudoAdjacency(PseudoChromsome_PseudoAdjacencyIterator *pseudoAdjacencyIterator);

/*
 * Duplicates the iterator.
 */
PseudoChromsome_PseudoAdjacencyIterator *pseudoChromosome_copyPseudoChromosomeIterator(PseudoChromsome_PseudoAdjacencyIterator *pseudoAdjacencyIterator);

/*
 * Destructs the iterator.
 */
void pseudoChromosome_destructPseudoAdjacencyIterator(PseudoChromsome_PseudoAdjacencyIterator *pseudoAdjacencyIterator);

#endif