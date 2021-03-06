#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <memory>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

const std::string WHITESPACE = " \n\r\t\f\v";

using namespace std;

class SmallShell;

class Command {
// TODO: Add your data members
 protected:
    vector<string> cmd_params;
    const string cmdText;
 public:
  Command(const string&);

    const string &getCmdText() const;

    virtual ~Command(){};
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  virtual bool isExternalCMD() = 0;
  bool isBackgroundCMD();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);


    virtual ~BuiltInCommand() {}

    bool isExternalCMD() override;
};

class ExternalCommand : public Command {
protected:
    pid_t pid;
 public:
  ExternalCommand(const char* cmd_line, SmallShell &smash);
  virtual ~ExternalCommand() {}
  void execute() override;
  bool isExternalCMD() override;

    pid_t getPid() const;

    void setPid(pid_t pid);
};


class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};



class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
public:
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};



class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
   int jobID;
   pid_t pid;
   int insertTime;
   string cmd;
   bool isStopped;

  public:
      JobEntry(int jobId, pid_t pid, int insertTime, const string &cmd, bool isStopped);

      int getInsertTime() const;

      const string &getCmd() const;

      bool getStopped() const;

      pid_t getPid() const;

      int getJobId() const;

      void setStopped(bool mode);

  };
 // TODO: Add your data members
private:
 vector<JobEntry> jList = vector<JobEntry>();
    int maxJobID;
public:
  JobsList();
  ~JobsList();
  void addJob(const string& cmdTxt, bool isStopped, pid_t pid);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry & getJobById(int jobId);
  void removeJobById(int jobId);
  void removeJobByPID(pid_t pid);
  JobEntry & getLastJob();
  JobEntry &getLastStoppedJob();
  void findMaxJobID();
  static bool compareEntry(JobEntry entry1, JobEntry entry2);
  bool isJobIdInList(int jobId) const;
  bool empty() const;
  bool noStoppedJobs() const;
  void printAllKilledJobs() const;
  void addJobAtJobId(const string &cmdText, int jobId, pid_t pid);

  // TODO: Add extra methods or modify exisitng ones as needed
};
class TimedJobEntry {
    // TODO: Add your data members
    int duration;
    pid_t pid;
    int insertTime;
    string cmd;
public:
    int getDuration() const;

    TimedJobEntry(int duration, pid_t pid, int insertTime, const string &cmd);

    pid_t getPid() const;

    int getInsertTime() const;

    const string &getCmd() const;


};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList& jobslist;
 public:
  JobsCommand(const char* cmd_line, JobsList& jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class CatCommand : public BuiltInCommand {
 public:
  CatCommand(const char* cmd_line);
  virtual ~CatCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  string plastPwd = "";
  string prompt;
  vector<TimedJobEntry> timeOutJobs;
  vector<int> alarms;
  bool inPipeCMD = false;
public:
    bool isInPipeCmd() const;

    void setInPipeCmd(bool inPipeCmd);

public:
    int getFgJobId() const;

    void setFgJobId(int fgJobId);

    const string &getFgJobCmd() const;

    void setFgJobCmd(const string &fgJobCmd);

    void addTimeOutJob(string cmd_text, int start_time, pid_t pid, int duration);

    void removeTimeOutJobByPID(pid_t pid);

    void setAlarm(int duration);

    void setNextAlarm();

private:
    JobsList jobslist;
    pid_t fgJobPid = 0;
    int fgJobId = 0;
    string fgJobCMD;

public:
    void setFgJobPid(pid_t fgJobPid);

    string getPlastPwd() const;

    void setPlastPwd(string plastPwd);
    //should be initiated to NULL?
    pid_t getFgJobPid() const;

    string getPrompt() const;

    void setPrompt(string prompt);

    JobsList &getJobslist();

    void removeTimedOutJob();

private:
    SmallShell();
 public:
  shared_ptr<Command> CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  void pipeFunction(string cmd_line);
  void redirectFunction(string cmd_line);
  // TODO: add extra methods as needed
};

class CHPromptCommand : public BuiltInCommand {
    SmallShell &smash;
// TODO: Add your data members
 public:
    CHPromptCommand(const char *cmd_line, SmallShell &smash);
    virtual ~CHPromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    SmallShell& smash;
// TODO: Add your data members public:
public:
    ChangeDirCommand(const char* cmd_line, SmallShell &smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};
#endif //SMASH_COMMAND_H_
