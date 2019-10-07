include Makefile.FLTK

RM    = rm -f
SHELL = /bin/sh
.SILENT:

# Executables
ALL = main$(EXEEXT)

# build Release
Release: MKDIR_Release_src clean Logger KeysHandling LiquiditiesHandling InternalThread MessageBuilder RequestBuilder MessageProcessor ConnectionHandling NewAccountW TransferLiquiW ShowClaimsW TransRequestsW NewTransRequestW ShowTransRqstsW ShowTransInfoW ShowKeyInfoW ShowLiquiInfoW OperationProposalW CreoleCheatSheatW FindRefThreadsW ShowDecThreadW FeeAndStakeCalcW RefereeApplicationW RefereeNoteW ShowRefInfoW DeleteLiquiW ExchangeOffersW NewExchangeOfferW ShowExchangeOffersW AcceptOfferW ShowLineageInfoW NotaryApplicationW ShowNotaryInfoW FindNotaryThreadsW NotaryNoteW AssignLiquiNameW Fl_Html_Formatter Fl_Html_Object Fl_Html_Parser Fl_Html_View $(ALL)

MKDIR_Release_src:
	mkdir -p obj/Release/src

Logger: src/Logger.cpp include/Logger.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

KeysHandling: src/KeysHandling.cpp include/KeysHandling.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

LiquiditiesHandling: src/LiquiditiesHandling.cpp include/LiquiditiesHandling.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

InternalThread: src/InternalThread.cpp include/InternalThread.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

MessageBuilder: src/MessageBuilder.cpp include/MessageBuilder.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

RequestBuilder: src/RequestBuilder.cpp include/RequestBuilder.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

MessageProcessor: src/MessageProcessor.cpp include/MessageProcessor.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ConnectionHandling: src/ConnectionHandling.cpp include/ConnectionHandling.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

NewAccountW: src/NewAccountW.cpp include/NewAccountW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

TransferLiquiW: src/TransferLiquiW.cpp include/TransferLiquiW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowClaimsW: src/ShowClaimsW.cpp include/ShowClaimsW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

TransRequestsW: src/TransRequestsW.cpp include/TransRequestsW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

NewTransRequestW: src/NewTransRequestW.cpp include/NewTransRequestW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowTransRqstsW: src/ShowTransRqstsW.cpp include/ShowTransRqstsW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowTransInfoW: src/ShowTransInfoW.cpp include/ShowTransInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowKeyInfoW: src/ShowKeyInfoW.cpp include/ShowKeyInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowLiquiInfoW: src/ShowLiquiInfoW.cpp include/ShowLiquiInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

OperationProposalW: src/OperationProposalW.cpp include/OperationProposalW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

CreoleCheatSheatW: src/CreoleCheatSheatW.cpp include/CreoleCheatSheatW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

FindRefThreadsW: src/FindRefThreadsW.cpp include/FindRefThreadsW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowDecThreadW: src/ShowDecThreadW.cpp include/ShowDecThreadW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

FeeAndStakeCalcW: src/FeeAndStakeCalcW.cpp include/FeeAndStakeCalcW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

RefereeApplicationW: src/RefereeApplicationW.cpp include/RefereeApplicationW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

RefereeNoteW: src/RefereeNoteW.cpp include/RefereeNoteW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowRefInfoW: src/ShowRefInfoW.cpp include/ShowRefInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

DeleteLiquiW: src/DeleteLiquiW.cpp include/DeleteLiquiW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ExchangeOffersW: src/ExchangeOffersW.cpp include/ExchangeOffersW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

NewExchangeOfferW: src/NewExchangeOfferW.cpp include/NewExchangeOfferW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowExchangeOffersW: src/ShowExchangeOffersW.cpp include/ShowExchangeOffersW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

AcceptOfferW: src/AcceptOfferW.cpp include/AcceptOfferW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowLineageInfoW: src/ShowLineageInfoW.cpp include/ShowLineageInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

NotaryApplicationW: src/NotaryApplicationW.cpp include/NotaryApplicationW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

ShowNotaryInfoW: src/ShowNotaryInfoW.cpp include/ShowNotaryInfoW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

FindNotaryThreadsW: src/FindNotaryThreadsW.cpp include/FindNotaryThreadsW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

NotaryNoteW: src/NotaryNoteW.cpp include/NotaryNoteW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

AssignLiquiNameW: src/AssignLiquiNameW.cpp include/AssignLiquiNameW.h
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -I../EntriesHandling/include -I../cryptopp610 -I../curl-7.63.0/include -I../NME-distrib/Src -c src/$@.cpp -o obj/Release/src/$@.o -std=c++11

Fl_Html_Formatter: src/Fl_Html_Formatter.cxx include/Fl_Html_Formatter.H 
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -c src/$@.cxx -o obj/Release/src/$@.o -std=c++11

Fl_Html_Object: src/Fl_Html_Object.cxx include/Fl_Html_Object.H 
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -c src/$@.cxx -o obj/Release/src/$@.o -std=c++11

Fl_Html_Parser: src/Fl_Html_Parser.cxx include/Fl_Html_Parser.H 
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -c src/$@.cxx -o obj/Release/src/$@.o -std=c++11

Fl_Html_View: src/Fl_Html_View.cxx include/Fl_Html_View.H 
	$(CXX) -I.. $(CXXFLAGS) -Iinclude -c src/$@.cxx -o obj/Release/src/$@.o -std=c++11

# clean everything
clean:
	$(RM) $(ALL)
	$(RM) *.o
	$(RM) core

