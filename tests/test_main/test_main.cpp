#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <iostream>

int
main(int argc, char *argv[])
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest(registry.makeTest());
#if 0
    bool wasSuccessful = runner.run("", false);
    return wasSuccessful? 0: 1;
#else
    CppUnit::TestResult testresult;
    CppUnit::TestResultCollector collectedresults;
    testresult.addListener(&collectedresults);
    CppUnit::BriefTestProgressListener progress;
    testresult.addListener(&progress);
    runner.run(testresult);
    CppUnit::CompilerOutputter compileroutputter(&collectedresults, std::cerr);
    compileroutputter.write();
    return collectedresults.wasSuccessful()? 0: 1;
#endif
}

// vim:ts=4:sts=4:sw=4:et:
