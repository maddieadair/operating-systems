#include <iostream>
#include <vector>
#include <queue>
#include <iomanip>
#include <fstream>

using namespace std;

struct Event {
    string command;
    int time;

    Event(string command, int time){
        this->command = command;
        this->time = time;
    };
};

struct Process {
    int PID;
    float time;
    string operation;
    int logReads;
    int physWrites;
    int physReads;
    int buffer;
};

int BSIZE;
vector<Event> inputTable;
vector<vector<int>> processTable;

bool CPUisEmpty = true;
bool SSDisEmpty = true;

float clockTime = 0;

queue<Process> readyQueue;
queue<Process> ssdQueue;

struct compareProcess
{
    bool operator()(Process const &e1, Process const &e2)
    {
        if (e1.time != e2.time) {
            return e1.time > e2.time;
        }
        else {
            return e1.operation > e2.operation;
        }
    }
};

priority_queue<Process, vector<Process>, compareProcess> mainQueue;

//  -------- function declarations -----------------------------------------------------------------------------------------------------------------

void arrival(Process& p);
void coreRequest(Process& p);
void coreCompletion(Process& p);
int calculateBuffer(int bytesNeeded, int currBSIZE);
void SSDRequest(Process& p);
void SSDCompletion(Process &p);
void IOCompletion(Process& p);
void print(Process p);


// ------------------------------------------------------------------------------------------------------------------------------------------------

int main() {

    string eventType;
    int eventTime;

    // create an Event for each line of input and push them into inputTable
    while (cin >> eventType >> eventTime){
        if (eventType == "BSIZE"){
            BSIZE = eventTime;    
        }
        Event e(eventType, eventTime);
        inputTable.push_back(e);
    }

    int count = 0;
    int processCount = -1; // counter for counting # of processes

    // loop through inputTable to get startLine, endLine, currLine for each process in the Process table
    for (int i = 0; i < inputTable.size(); i++){
        if (inputTable[i].command == "START"){

            processCount++;
            vector<int> tempArr(5);
            tempArr[0] = processCount; // Process ID (PID) = processCount
            tempArr[1] = i; // Start Line
            tempArr[3] = -1; // Initialize Current Line to -1
            tempArr[4] = 0; // Initialize State to 0 for "Not Started"

            // if current line command = "START" keep going until it finds another "START" to get End Line
            int k = i+1;
            while (inputTable[k].command != "START" && k < inputTable.size()){
                count++;
                k++;
            }

            // End Line = Start Line + line Count
            tempArr[2] = i + count;
            count = 0; // reset line count 

            processTable.push_back(tempArr); // push tempArr (PID, startLine, endLine, currLine, state) to Process Table
        }
    }

    // create a Process for each line in Process Table
    for (int i = 0; i < processTable.size(); i++){
        int index = processTable[i][0]; // index = Process ID
        int start = processTable[i][1]; // start = Process Start Line

        Process p;
        p.logReads = 0;
        p.physReads = 0;
        p.physWrites = 0;
        p.buffer = 0;
        p.PID = index;
        p.operation = inputTable[start].command; // get first command of the Process in inputTable
        p.time = inputTable[start].time; // get the first time of the Process in inputTable

        mainQueue.push(p); // push them to the Main Queue to initialize it
    }

    // while Main Queue is not empty, pop the top, set clockTime = top.time()
    // and process the process
    while (!mainQueue.empty()) { 
        Process top = mainQueue.top();
        mainQueue.pop();

        clockTime = top.time;

        if (top.operation == "START"){
            arrival(top);
        }
        else if (top.operation == "CORE"){
            coreCompletion(top);
        }
        else if (top.operation == "WRITE" || top.operation == "READ"){
            SSDCompletion(top);
        }
        else if (top.operation == "INPUT" || top.operation == "DISPLAY") {
            IOCompletion(top);
        }
    } 

    return 0;
}

//  -------- functions -----------------------------------------------------------------------------------------------------------------

void arrival(Process& p){
    // increment the process table current line 
    processTable[p.PID][3] = processTable[p.PID][1] + 1;
    // get the next command (next line) from the input table (will always be CORE)
    int nextInputIndex = processTable[p.PID][3];

    // change the Process p's operation and time to the next command
    p.operation = inputTable[nextInputIndex].command;
    p.time = inputTable[nextInputIndex].time;

    // send Core Request for p
    coreRequest(p);
}

void coreRequest(Process& p){
    // if the CPU is empty
    if (CPUisEmpty) {
        CPUisEmpty = false; // set the CPU to full
        processTable[p.PID][4] = 1; // change process table status to RUNNING (1)

        p.time = clockTime + p.time; // Find completion time = clockTime + Time Needed 
        mainQueue.push(p); // push process back into Main Queue using it's completion time
    }
    else if (!CPUisEmpty){ // CPU is Empty
        processTable[p.PID][4] = 2; // change process table status to READY (2)
        readyQueue.push(p); // push process to Ready Queue with its Time Needed
    }
}

void coreCompletion(Process& p){
    CPUisEmpty = true; // set CORE to empty

    // process anything in the Ready Queue first
    if (!readyQueue.empty()) { // if Ready Queue is not Empty
        Process top = readyQueue.front(); // get top of Ready Queue
        readyQueue.pop(); // pop the top

        coreRequest(top); // get Core Request for it
    }
    // if End Line == Current Line
    if (processTable[p.PID][2] == processTable[p.PID][3]) {
        processTable[p.PID][4] = -1; // terminate
        print(p); // print
    }
    else { // Ready Queue is Empty
        processTable[p.PID][3]++; // Increment Current Line by 1
        int nextLine = processTable[p.PID][3]; // get Next Line/Command (currentLine)

        // if next/current Command is READ or WRITE, get SSD Request
        if (inputTable[nextLine].command == "READ" || inputTable[nextLine].command == "WRITE") {
            SSDRequest(p);
        }
        // if command is INPUT or DISPLAY
        else if (inputTable[nextLine].command == "INPUT" || inputTable[nextLine].command == "DISPLAY") {
            p.operation = inputTable[nextLine].command; // change process operation to the command
            p.time = clockTime + inputTable[nextLine].time; // get Process completion time
            processTable[p.PID][4] = 3; // set process status to BLOCKED
            mainQueue.push(p); // push process to Main Queue
        }
    }
}

int calculateBuffer(int bytesNeeded, int currBSIZE){
    int finalBufferSize;

    if (bytesNeeded <= currBSIZE){ // if Requested Bytes is less than current buffer size
        finalBufferSize = currBSIZE - bytesNeeded; // decrease current buffer size by requested bytes
    }
    else { // requested bytes is more than current buffer size
        int bytesMissing = bytesNeeded - currBSIZE; // get # of Missing Bytes
        int bytesBrought;
        if (bytesMissing % BSIZE == 0){
            bytesBrought = bytesMissing;
        }
        else {
            bytesBrought = (bytesMissing / BSIZE + 1) * BSIZE;
        }

        finalBufferSize = bytesBrought - bytesMissing; // get final buffer size
    }

    return finalBufferSize;
}

void SSDRequest(Process& p) {
    // get READ/WRITE command in inputTable from current process table line
    int ssdLine = processTable[p.PID][3];
    Event e = inputTable[ssdLine];
    
    if (e.command == "READ"){ // if the command is READ
        if (p.buffer >= e.time) { // if current buffer size > requested bytes
            p.logReads++; // increase Logical Reads by 1
            p.buffer = calculateBuffer(e.time, p.buffer); // get new buffer size

            processTable[p.PID][3]++; // increment process table current line by 1
            int nextLine = processTable[p.PID][3]; // get next command/new current line
            p.time = inputTable[nextLine].time; // get new process time
            p.operation = inputTable[nextLine].command; // get new process command

            coreRequest(p); // get core request for process p
            
        }
        else {
            p.operation = "READ"; 

            processTable[p.PID][4] = 3; // set process state to Blocked

            if (SSDisEmpty) { // if the SSD is empty
                p.time = clockTime + 0.1; // set process time = clockTime + 0.1
                p.physReads++; // increment physical reads by 1
                p.buffer = calculateBuffer(e.time, p.buffer); // get new buffer size
                mainQueue.push(p); // push process to main queue
                SSDisEmpty = false; // set ssd queue to full
            }
            else { // SSD is full
                p.time = e.time; // get process time
                ssdQueue.push(p); // push to SSD queue
            }
        }
    }
    else if (e.command == "WRITE"){ // command is WRITE
        p.physWrites++; // increment physical writes by 1
        p.time = clockTime + 0.1; // process time = clockTime + 0.1
        p.operation = "WRITE";
        processTable[p.PID][4] = 3; // set process state to Blocked

        if (SSDisEmpty){ // is SSD is empty
            mainQueue.push(p); // push to main queue
            SSDisEmpty = false; // set SSD to full
        }
        else { // SSD is full
            ssdQueue.push(p); // push to SSD
        }
    }
} 

void SSDCompletion(Process& p) {
    SSDisEmpty = true; // set SSD to empty
    processTable[p.PID][4] = 2; // change blocked process status back to READY (2)
    if (!ssdQueue.empty()) { // is SSD queue is full

        Process f = ssdQueue.front(); // get SSD queue front
        ssdQueue.pop(); // pop font

        SSDRequest(f); // get SSD request
    }
    else { // SSD queue is empty
        processTable[p.PID][3]++; // increment process table current line by 1
        int nextLine = processTable[p.PID][3]; // get next command/new current line command

        p.operation = inputTable[nextLine].command;
        p.time = inputTable[nextLine].time;

        coreRequest(p); // get core request for new process bc it will always be CORE
    }
} 

void IOCompletion(Process& p){
    processTable[p.PID][4] = 2; // change blocked process status back to READY (2)
    processTable[p.PID][3]++; // increment process table current line by 1
    int nextLine = processTable[p.PID][3]; // get new command/new current line 

    p.time = inputTable[nextLine].time;
    p.operation = inputTable[nextLine].command;

    coreRequest(p);  // get core request bc it will always be CORE
}


void print(Process p) {
    cout << fixed << setprecision(1) << "Process " << p.PID << " terminates at time " << p.time << " ms." << endl;
    cout << "it performed " << p.physReads << " physical read(s), " << p.logReads << " in-memory read(s), and " << p.physWrites << " physical write(s)." << endl;
    cout << "Process Table:" << endl;

    for (int i = 0; i < processTable.size(); i++){
        if (i == p.PID){
            cout << "Process " << i << " is TERMINATED." << endl;
        }
        else {
            if (processTable[i][4] == 1){ // RUNNING
                cout << "Process " << i << " is RUNNING." << endl;
            }
            else if (processTable[i][4] == 2){ // READY
                cout << "Process " << i << " is READY." << endl;
            }
            else if (processTable[i][4] == 3){ // BLOCKED
                cout << "Process " << i << " is BLOCKED." << endl;
            } 
        }
    }
    cout << endl;
} 
