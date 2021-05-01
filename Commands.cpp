#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

vector<string> _cmdLineToParams(const string& cmd_line){
    vector<string> params;
    std::istringstream iss(_trim(string(cmd_line)));
    for(string s; iss >> s; ) {
        params.push_back(s);
    }
    params.erase(params.begin());
    return params;
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(string& cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


SmallShell::SmallShell() : plastPwd(nullptr), prompt("smash") {
}

SmallShell::~SmallShell() {
    delete plastPwd;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord.compare("chprompt") == 0) {
        return new CHPromptCommand(cmd_line, getInstance());
    }
    if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, getInstance());
    }
    if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, getJobslist());
    }
    if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line);
    }
    if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line);
    }
    if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line);
    }
    if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line);
    }
    return new ExternalCommand(cmd_line, getInstance());
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
}

void SmallShell::executeCommand(const char *cmd_line) {
    if(string(cmd_line).find("|") != string::npos) {
        pipeFunction(cmd_line);
    } else if(string(cmd_line).find(">") != string::npos){
        redirectFunction(cmd_line);
    } else {
        Command *cmd = CreateCommand(cmd_line);
        jobslist.removeFinishedJobs();
        if (cmd->isExternalCMD()) {
            pid_t pid = fork();
            if (pid == 0) {
                setpgrp();
                cmd->execute();
            } else if (pid == -1) {
                perror("smash error: fork failed");
            } else {
                if (cmd->isBackgroundCMD()) {
                    jobslist.removeFinishedJobs();
                    jobslist.addJob(cmd->getCmdText(), false, pid);
                } else {
                    fgJobPid = pid;
                    fgJobCMD = cmd->getCmdText();
                    waitpid(pid, nullptr, WUNTRACED);
                    fgJobPid = 0;
                }
            }
        } else {
            cmd->execute();
        }
    }// Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPrompt() const {
    return prompt;
}

void SmallShell::setPrompt(string prompt) {
    SmallShell::prompt = prompt;
}

char *SmallShell::getPlastPwd() const {
    return plastPwd;
}

void SmallShell::setPlastPwd(char *plastPwd) {
    SmallShell::plastPwd = plastPwd;
}

JobsList &SmallShell::getJobslist() {
    return jobslist;
}

pid_t SmallShell::getFgJobPid() const {
    return fgJobPid;
}

int SmallShell::getFgJobId() const {
    return fgJobId;
}

void SmallShell::setFgJobId(int fgJobId) {
    SmallShell::fgJobId = fgJobId;
}

const string &SmallShell::getFgJobCmd() const {
    return fgJobCMD;
}

void SmallShell::setFgJobCmd(const string &fgJobCmd) {
    fgJobCMD = fgJobCmd;
}

void SmallShell::setFgJobPid(pid_t fgJobPid) {
    SmallShell::fgJobPid = fgJobPid;
}

void SmallShell::pipeFunction(string cmd_line) {
    int fd[2];
    if(pipe(fd) == -1){
        perror("smash error: pipe failed");
    }
    size_t pipe_pos = cmd_line.find("|");
    bool isErrorPipe = (cmd_line.find("|&") != string::npos);
    string _cmd1 = cmd_line.substr(0, pipe_pos);
    string _cmd2 = isErrorPipe ? cmd_line.substr(pipe_pos + 2) : cmd_line.substr(pipe_pos + 1);

    Command* cmd1 = CreateCommand(_cmd1.c_str());
    Command* cmd2 = CreateCommand(_cmd2.c_str());
        pid_t pid1 = fork();
        if(pid1 == 0){
            setpgrp();
            if(isErrorPipe){
                if(dup2(fd[1], 2) == -1){
                    perror("smash error: dup2 failed");
                    return;
                }
            } else {
                if(dup2(fd[1], 1) == -1){
                    perror("smash error: dup2 failed");
                    return;
                }
            }
            if(close(fd[0]) == -1){
                perror("smash error: close failed");
                return;
            }
            if(close(fd[1]) == -1){
                perror("smash error: close failed");
                return;
            }
            cmd1->execute();
        } else if(pid1 == -1) {
            perror("smash error: fork failed");
        }
        pid_t pid2 = fork();
        if(pid2 == 0){
            setpgrp();
            if(dup2(fd[0], 0) == -1){
                perror("smash error: dup2 failed");
                return;
            }
            if(close(fd[0]) == -1){
                perror("smash error: close failed");
                return;
            }
            if(close(fd[1]) == -1){
                perror("smash error: close failed");
                return;
            }
            cmd2->execute();
        } else if(pid2 == -1) {
            perror("smash error: fork failed");
        } else{
            if(close(fd[0]) == -1){
                perror("smash error: close failed");
                return;
            }
            if(close(fd[1]) == -1){
                perror("smash error: close failed");
                return;
            }
            waitpid(pid1, nullptr, WUNTRACED);
            waitpid(pid2, nullptr, WUNTRACED);
    }


}

void SmallShell::redirectFunction(string cmd_line) {
    size_t red_pos = cmd_line.find(">");
    bool isAppend = (cmd_line.find(">>") != string::npos);
    string _cmd = cmd_line.substr(0, red_pos);
    Command* cmd = CreateCommand(_cmd.c_str());
    cmd_line = _trim(cmd_line);
    _removeBackgroundSign(cmd_line);
    red_pos = cmd_line.find(">");
    string file = isAppend ? _trim(cmd_line.substr(red_pos + 2)) : _trim(cmd_line.substr(red_pos + 1));
    int fd;
    if(isAppend) {
        fd = open(file.c_str(), O_RDWR | O_CREAT | O_APPEND);
    } else {
        fd = open(file.c_str(), O_RDWR | O_CREAT);
    }
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
    int oldOutFd = dup(1);
    if(oldOutFd == -1){
        perror("smash error: dup failed");
        return;
    }
    int dup2Res = dup2(fd, 1);
    if(dup2Res == -1){
        perror("smash error: dup2 failed");
        return;
    }


    pid_t pid = fork();
    if(pid == 0){
        setpgrp();
        cmd->execute();
    } else if(pid == -1) {
        perror("smash error: fork failed");
    } else{
        waitpid(pid, nullptr, WUNTRACED);
        dup2Res = dup2(oldOutFd, 1);
        if(dup2Res == -1){
            perror("smash error: dup2 failed");
            return;
        }
    }


}

CHPromptCommand::CHPromptCommand(const char *cmd_line, SmallShell &smash)
        : BuiltInCommand(cmd_line), smash(smash) {}

void CHPromptCommand::execute() {
    if(!cmd_params.empty()){
        smash.setPrompt(*cmd_params.begin()); //isn't bad playing with iterator?
    } else {

        smash.setPrompt("smash");
    }


}

Command::Command(const string& cmd_line) : cmdText(cmd_line) {
    cmd_params = _cmdLineToParams(cmd_line);
}

bool Command::isBackgroundCMD() {
//    cout << cmd_params.back() << endl;
    return cmd_params.empty() ? false : cmd_params.back().back() == '&';
}

const string &Command::getCmdText() const {
    return cmdText;
}

bool BuiltInCommand::isExternalCMD() {return false;}
bool ExternalCommand::isExternalCMD() {return true;}
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
   if(!cmd_params.empty() && cmd_params.back() == "&") cmd_params.pop_back();
   else if(!cmd_params.empty() && cmd_params.back().back() == '&') cmd_params.back().pop_back();
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
 }

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

void GetCurrDirCommand::execute() {
    char* currentDir = get_current_dir_name();
    if(currentDir == NULL){
        perror("smash error: get_current_dir_name failed");
        return;
    }
    cout << currentDir << endl;
    delete currentDir;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, SmallShell& smash) : BuiltInCommand(cmd_line), smash(smash)  {

}

void ChangeDirCommand::execute() {
    char* currentDir = get_current_dir_name();
    if(currentDir == NULL){
        perror("smash error: get_current_dir_name failed");
        return;
    }
    char *plastPwd = smash.getPlastPwd();
    if (cmd_params.size() == 0) { //What happens if size==0?
        return;
    }
    if (cmd_params.size() > 1) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (cmd_params.front() == "-") {
        if (plastPwd != nullptr) {
            if(chdir(plastPwd) != -1){
                delete plastPwd;
                smash.setPlastPwd(currentDir);
            } else {
                perror("smash error: chdir failed");
            }
            return;
        }
        else {
            cerr << "smash error: cd: OLDPWD not set" << endl;  // if null
            return;
        }
    }
    else {
        delete plastPwd;
        smash.setPlastPwd(currentDir);
        char* path = new char [cmd_params.front().length()+1];
        strcpy(path, cmd_params.front().c_str());
        //should null terminator be removed?
        if(chdir(path) == -1){
            perror("smash error: chdir failed");
        }
        delete[] path;
        return;
    }
}

ExternalCommand::ExternalCommand(const char *cmd_line, SmallShell &smash): Command(cmd_line) {
}


void ExternalCommand::execute() {
    const char* arguments [4];
    //const char* programName = "sh";
    arguments[0] = "bash";
    arguments[1] = "-c";
    string s = cmdText;
    string args = _trim(s);
    _removeBackgroundSign(args);
    arguments[2] = args.c_str();
    arguments[3] = NULL;
//    for (int j = 0;  j < cmd_params.size()+1;  ++j) {     // copy args
//        arguments[j+1] = cmd_params[j].c_str();
//    }
//    arguments[cmd_params.size()+1] = nullptr;                    //nullptr or NULL?
    const char* path = "/bin/bash";
    execv(path, (char*const*) arguments);
}

JobsList::JobEntry::JobEntry(int jobId, pid_t pid, int insertTime, const string &cmd, bool isStopped) : jobID(jobId), pid(pid),
                                                                                        insertTime(insertTime),
                                                                                        cmd(cmd), isStopped(isStopped) {}

int JobsList::JobEntry::getJobId() const {
    return jobID;
}

int JobsList::JobEntry::getInsertTime() const {
    return insertTime;
}

const string &JobsList::JobEntry::getCmd() const {
    return cmd;
}

bool JobsList::JobEntry::getStopped() const {
    return isStopped;
}

pid_t JobsList::JobEntry::getPid() const {
    return pid;
}

void JobsList::JobEntry::setStopped(bool mode) {
    isStopped = mode;

}

JobsList::JobsList() :  maxJobID(0)  {

}

JobsList::~JobsList() {

}

void JobsList::addJob( const string& cmdTxt, bool isStopped, pid_t pid) {
    time_t t = time(nullptr);
    if(t == -1){
        perror("smash error: time failed");
        return;
    }
    jList.push_back(JobEntry(++maxJobID, pid, t, cmdTxt, isStopped ));

}

void JobsList::printJobsList() {
    //[<job-id>] <command> : <process id> <seconds elapsed> (stopped)
    sort(jList.begin(), jList.end(), compareEntry);
    for (auto &entry: jList) {
        cout <<'['<<entry.getJobId()<<"] "<<entry.getCmd()<<" : "<<entry.getPid()<<" "
        <<difftime(time(nullptr), entry.getInsertTime())<<" secs";
        if (entry.getStopped()) cout<<" (stopped)";
        cout<<endl;
    }
}

void JobsList::killAllJobs() {
    for (auto &entry: jList) {
        if(kill(entry.getPid(), SIGKILL) == -1)
            perror("smash error: waitpid failed");
    }
}

void JobsList::removeFinishedJobs() {
    if(jList.empty()) return;
    int status;
    pid_t jobStatus = waitpid(-1,&status, WNOHANG);
    while(jobStatus != 0){
        if(jobStatus == -1){
            perror("smash error: waitpid failed");
            return;

        }
        removeJobByPID(jobStatus);
        if (jList.empty()){
            maxJobID = 0;
            return;
        }
        jobStatus = waitpid(-1, &status, WNOHANG);
    }
//    vector<int> toRemove;
//    for (auto &entry: jList) {
//        int status;
//        pid_t jobStatus = waitpid(entry.getPid(), &status, WNOHANG);
//        if(jobStatus == -1){
//            perror("smash error: waitpid failed");
//            return;
//        }
//        if(jobStatus == entry.getPid() && WIFEXITED(status)){
//            toRemove.push_back(entry.getJobId());
//        }
//    }
//
//    for (auto &i: toRemove) {
//        removeJobById(i);
//    }

}

JobsList::JobEntry &JobsList::getJobById(int jobId) {
    for (auto &entry: jList) {
        if (entry.getJobId()==jobId) return entry;
    }
    throw runtime_error("no JobID found");
}

void JobsList::removeJobById(int jobId) {
    for (auto it = jList.begin(); it != jList.end(); ++it) {
        if (it->getJobId() == jobId){
            jList.erase(it);
            findMaxJobID();
            return;
        }
    }
}

void JobsList::removeJobByPID(pid_t pid) {
    for (auto it = jList.begin(); it != jList.end(); ++it) {
        if (it->getPid() == pid){
            jList.erase(it);
            findMaxJobID();
            return;
        }
    }
}
JobsList::JobEntry &JobsList::getLastJob() {
    return getJobById(maxJobID);
}

JobsList::JobEntry &JobsList::getLastStoppedJob() {
    int jobID = 1;
    for (auto &entry: jList) {
        if (entry.getStopped()) jobID = entry.getJobId() ;
    }
    return getJobById(jobID);
}

bool JobsList::compareEntry(JobEntry entry1, JobEntry entry2) {
    return (entry1.getJobId() < entry2.getJobId());
}


bool JobsList::isJobIdInList(int jobId) const {
    for (auto &entry: jList) {
        if (entry.getJobId()==jobId) return true;
    }
    return false;
}

void JobsList::findMaxJobID() {
    maxJobID = 0;
    for (auto &entry: jList) {
        if (entry.getJobId() > maxJobID) maxJobID = entry.getJobId();
    }
}

bool JobsList::empty() const {
    return jList.empty();
}

bool JobsList::noStoppedJobs() const {
    for (auto &entry: jList) {
        if (entry.getStopped()) return false;
    }
    return true;
}

void JobsList::printAllKilledJobs() const {
    cout << "smash: sending SIGKILL signal to " << jList.size() << " jobs" << endl;
    for (auto &entry: jList) {
        cout << entry.getPid() << ": " << entry.getCmd() << endl;
    }

    cout << "Linux-shell:" << endl;


}

void JobsList::addJobAtJobId(const string &cmdText, int jobId, pid_t pid) {
    time_t t = time(nullptr);
    if(t == -1){
        perror("smash error: time failed");
        return;
    }
    jList.push_back(JobEntry(jobId, pid, t, cmdText, true));


}

JobsCommand::JobsCommand(const char *cmd_line, JobsList &jobs) : BuiltInCommand(cmd_line), jobslist(jobs) {

}
void JobsCommand::execute() {
    jobslist.removeFinishedJobs();
    jobslist.printJobsList();
    return;
}

pid_t ExternalCommand::getPid() const {
    return pid;
}
void ExternalCommand::setPid(pid_t pid) {
    ExternalCommand::pid = pid;
}

bool invalid_arg(const string &str) {
    if(str.empty()) return true;
    for(int i = 0; i < str.length(); i++) {
        if(str[i] == '-' && i == 0) continue;
        if(str[i] < '0' || str[i] > '9') return true;
    }
    return false;
}

void KillCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if(cmd_params.size() != 2 || cmd_params[0][0] != '-' || invalid_arg(cmd_params[0].substr(1)) || invalid_arg(cmd_params[1])){
        cerr << "smash error: kill: invalid arguments" << endl;
    } else if(!smash.getJobslist().isJobIdInList(stoi(cmd_params[1]))){
        cerr << "smash error: kill: job-id " << cmd_params[1] << " does not exist" << endl;
    } else if(kill(smash.getJobslist().getJobById(stoi(cmd_params[1])).getPid(), stoi(cmd_params[0].substr(1))) == -1){
        perror("smash error: kill failed");
    } else{
        cout << "signal number " << cmd_params[0].substr(1) << " was sent to pid " << smash.getJobslist().getJobById(stoi(cmd_params[1])).getPid() << endl;
    }
}


KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    JobsList &jobsList = smash.getJobslist();
    if (cmd_params.size() > 1 || (cmd_params.size() == 1 && invalid_arg(cmd_params[0]))) {
        cerr << "smash error: fg: invalid arguments" << endl;
    } else if (cmd_params.empty() && jobsList.empty()) {
        cerr << "smash error: fg: jobs list is empty" << endl;

    } else if (cmd_params.size() == 1 && !jobsList.isJobIdInList(stoi(cmd_params[0]))) {
        cerr << "smash error: fg: job-id " << cmd_params[0] << " does not exist" << endl;
    } else if (cmd_params.size() == 1) {
        JobsList::JobEntry &job = jobsList.getJobById(stoi(cmd_params[0]));
        cout << job.getCmd() << " : " << job.getPid() << endl;
        smash.setFgJobId(job.getJobId());
        smash.setFgJobCmd(job.getCmd());
        smash.setFgJobPid(job.getPid());
        if (kill(job.getPid(), SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            if (waitpid(job.getPid(), NULL, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            } else {
                jobsList.removeJobById(job.getJobId());
            }
            smash.setFgJobPid(0);
            smash.setFgJobId(0);
        }
    } else {
        JobsList::JobEntry &job = jobsList.getLastJob();
        cout << job.getCmd() << " : " << job.getPid() << endl;
        smash.setFgJobId(job.getJobId());
        smash.setFgJobCmd(job.getCmd());
        smash.setFgJobPid(job.getPid());
        if (kill(job.getPid(), SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            if (waitpid(job.getPid(), NULL, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            } else {
                jobsList.removeJobById(job.getJobId());
            }
            smash.setFgJobPid(0);
            smash.setFgJobId(0);

        }


    }
}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
    SmallShell &smash = SmallShell::getInstance();
    JobsList &jobsList = smash.getJobslist();
    if (cmd_params.size() > 1 || (cmd_params.size() == 1 && invalid_arg(cmd_params[0]))) {
        cerr << "smash error: bg: invalid arguments" << endl;
    } else if (cmd_params.empty() && jobsList.noStoppedJobs()) {
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;

    } else if (cmd_params.size() == 1 && !jobsList.isJobIdInList(stoi(cmd_params[0]))) {
        cerr << "smash error: bg: job-id " << cmd_params[0] << " does not exist" << endl;
    } else if (cmd_params.size() == 1 && !jobsList.getJobById(stoi(cmd_params[0])).getStopped()){
        cerr << "smash error: bg: job-id " << cmd_params[0] << " is already running in the background" << endl;
    } else if (cmd_params.size() == 1) {
        JobsList::JobEntry &job = jobsList.getJobById(stoi(cmd_params[0]));
        cout << job.getCmd() << " : " << job.getPid() << endl;
        if (kill(job.getPid(), SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            job.setStopped(false);
        }

    } else {
        JobsList::JobEntry &job = jobsList.getLastStoppedJob();
        cout << job.getCmd() << " : " << job.getPid() << endl;
        if (kill(job.getPid(), SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            job.setStopped(false);
            }

        }


}



void BackgroundCommand::execute() {

}

QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}

void QuitCommand::execute() {
    JobsList &jobsList = SmallShell::getInstance().getJobslist();
    if(!cmd_params.empty() && cmd_params[0] == "kill"){
        jobsList.killAllJobs();
        jobsList.printAllKilledJobs();
    }
    exit(0);

}
