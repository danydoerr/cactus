import unittest
import sys

from sonLib.bioio import parseSuiteTestOptions
from sonLib.bioio import TestStatus

from cactus.shared.test import getCactusInputs_random
from cactus.shared.test import runWorkflow_multipleExamples

class TestCase(unittest.TestCase):
    def testCactus_RandomAlignmentsOnly(self):
        """Build mafs from cactusDisks containing no trees, faces or reference.
        """
        runWorkflow_multipleExamples(getCactusInputs_random, #Just for the alignments.
                                     testNumber=TestStatus.getTestSetup(), 
                                     makeMAFs=True,buildTrees=False,buildFaces=False,buildReference=False)
    
    def testCactus_Random(self):
        """Build mafs from cactusDisks containing trees, face and an reference.
        """
        runWorkflow_multipleExamples(getCactusInputs_random, 
                                     testNumber=TestStatus.getTestSetup(), 
                                     makeMAFs=True)
        
def main():
    parseSuiteTestOptions()
    sys.argv = sys.argv[:1]
    unittest.main()
        
if __name__ == '__main__':
    main()