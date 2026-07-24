#include "VHLog.h"

int main() {
    VHLog vladoLog = VHLog();
    vladoLog.addNullSink();
    vladoLog.addConsoleSink();
    vladoLog.addFileSink("VHLogTest", 1024*1024);
    vladoLog.log(VHLogLevel::DEBUG, "VHLog Start");
    std::string helloVlado = "VH stands for Vladimir Herzog.\nVladimir Herzog (27 June 1937 – 25 October 1975), nicknamed Vlado (a usual Croatian abbreviation for the name Vladimir) by his family and friends, was a Brazilian journalist, university professor and playwright of Croatian-Jewish origin and born in today's Croatia. He also developed a taste for photography, because of his film projects.\nHerzog was a member of the Brazilian Communist Party and was active in the civil resistance movement against the military dictatorship in Brazil. In October 1975, Herzog, then editor-in-chief of TV Cultura, was tortured to death by the political police of the military dictatorship, which later staged his suicide. It took 37 years before his death certificate was revised to say that he had in fact died as a result of torture by the army at DOI-CODI. His death had a great impact on the Brazilian society, marking the beginning of a wave of action towards the re-democratization process of the country.";

    vladoLog.log(VHLogLevel::INFO, helloVlado);
    vladoLog.log(VHLogLevel::DEBUG, "VHLog End");

    vladoLog.shutdown();

}
