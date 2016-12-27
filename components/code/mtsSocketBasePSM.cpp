#include <sawIntuitiveResearchKit/mtsSocketBasePSM.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>

mtsSocketBasePSM::mtsSocketBasePSM(const std::string &componentName, const double periodInSeconds,
                                   const std::string &ip, const unsigned int port, bool isServer) :
    mtsTaskPeriodic(componentName, periodInSeconds),
    mIsServer(isServer),
    mTimeServer(mtsComponentManager::GetInstance()->GetTimeServer()),
    mPacketsLost(0),
    mPacketsDelayed(0)
{
    Command.Socket = new osaSocket(osaSocket::UDP);
    Command.IpPort = port;
    State.Socket = new osaSocket(osaSocket::UDP);
    State.IpPort = port+1;
    IpAddress = ip;

    this->StateTable.AddData(mPacketsLost, "PacketsLost");
    this->StateTable.AddData(mPacketsDelayed, "PacketsDelayed");
    this->StateTable.AddData(mLoopTime, "LoopTime");
    this->StateTable.AddData(Command.Data.Header.Id, "CommandId");
    this->StateTable.AddData(State.Data.Header.Id, "StateId");


    mtsInterfaceProvided *interfaceProvided = AddInterfaceProvided("System");
    if(interfaceProvided){
        interfaceProvided->AddCommandReadState(this->StateTable, StateTable.PeriodStats, "GetPeriodStatistics");
        interfaceProvided->AddCommandReadState(this->StateTable, mPacketsLost, "GetPacketsLost");
        interfaceProvided->AddCommandReadState(this->StateTable, mPacketsDelayed, "GetPacketsDelayed");
        interfaceProvided->AddCommandReadState(this->StateTable, mLoopTime, "GetLoopTime");
        if(mIsServer){
            interfaceProvided->AddCommandReadState(this->StateTable, Command.Data.Header.Id, "GetLastReceivedPacketId");
            interfaceProvided->AddCommandReadState(this->StateTable, State.Data.Header.Id, "GetLastSentPacketId");
        } else {
            interfaceProvided->AddCommandReadState(this->StateTable, State.Data.Header.Id, "GetLastReceivedPacketId");
            interfaceProvided->AddCommandReadState(this->StateTable, Command.Data.Header.Id, "GetLastSentPacketId");
        }
    }
}

void mtsSocketBasePSM::Cleanup()
{
    Command.Socket->Close();
    State.Socket->Close();
}

void mtsSocketBasePSM::UpdateStatistics()
{
    int deltaPacket = 1;
    if (mIsServer) {
        mLoopTime = mTimeServer.GetRelativeTime() - Command.Data.Header.LastTimestamp;                
        if (Command.Data.Header.Id == 1) {
            State.Data.Header.Id = 1;
            mPacketsLost = 0;
            mPacketsDelayed = 0;
        } else {
            if (State.Data.Header.Id != 0) {
                deltaPacket = Command.Data.Header.Id - State.Data.Header.LastId;
            }
        }
    } else {
        mLoopTime = mTimeServer.GetRelativeTime() - State.Data.Header.LastTimestamp;                        
        if (State.Data.Header.Id == 1) {
            Command.Data.Header.Id = 1;
            mPacketsLost = 0;
            mPacketsDelayed = 0;
        } else {
            if (Command.Data.Header.Id != 0) {
                deltaPacket = State.Data.Header.Id - Command.Data.Header.LastId;
            }
        }
    }

    if (deltaPacket == 0) {
       mPacketsDelayed++;
    } else if (deltaPacket > 1) {
        mPacketsLost += (deltaPacket - 1);
    }
}