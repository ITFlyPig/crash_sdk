#include <jni.h>
#include <string>
#include <signal.h>
#include <android/log.h>
#include <libunwind-aarch64.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>


#define TAG "crash_sdk_jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

#define ERROR -1
#define SUCCESS 0;

//保存最终的日志信息
char stack_log[2048];
//线程名称
char * thread_name;

JavaVM *g_jvm;
jobject g_obj;
//是否停止
bool b_stop = false;

//保存之前的信号处理结构体
struct sigaction *p_sa_old;

//线程控制
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define THREAD_NAME_LENGTH 100

//信号最多个数
#define SIG_NUMBER_MAX 32

//定义了需要捕获处理的信号
#define SIG_CATCH_COUNT 7
static const int sig_arr[SIG_CATCH_COUNT] = {SIGABRT,  //进程发现错误或者调用了abort()
                                             SIGILL,   //非法指令
                                             SIGTRAP,  //跟踪陷阱信号，由断点指令或其它trap指令产生
                                             SIGBUS,   //不存在的物理地址
                                             SIGFPE,   //浮点运算错误
                                             SIGSEGV}; //段地址错误，比如空指针、数组越界、野指针等





/**
 * 追加数据
 * @param des
 * @param src
 */
static void append(char *des, char *src) {
    if (des == nullptr || src == nullptr) return;
    pthread_mutex_lock(&mtx);
    strcat(des, src);
    strcat(des, "\n");
    pthread_mutex_unlock(&mtx);
}


/**
 * 使用unwind解析堆栈
 * @return
 */
static int slow_backtrace() {
    unw_context_t uc;
    unw_getcontext (&uc);

    unw_cursor_t cursor;
    unw_word_t pc;

    if (unw_init_local(&cursor, &uc) < 0)
        return ERROR;

    while (unw_step(&cursor) > 0) {

        if (unw_get_reg(&cursor, UNW_REG_IP, &pc) < 0)
            return ERROR;
        //尝试获取动态库的信息
        Dl_info info;
        if (dladdr((void *) pc, &info) != 0) {
            void *const nearest = info.dli_saddr;
            //相对偏移地址
            const uintptr_t addr_relative =
                    ((uintptr_t) pc - (uintptr_t) info.dli_fbase);
            char log[1024];
            snprintf(log, sizeof(log), "at %s(%s）", info.dli_fname, info.dli_sname);
            append(stack_log, log);
//            addr2line()
        }
    }
    return SUCCESS;
}


/**
 * 对于不同的信号和code 返回不同的描述
 * @param sig   信号
 * @param code  信号code
 * @return
 */
static const char *sig_desc(int sig, int code) {
    switch (sig) {
        case SIGILL:
            switch (code) {
                case ILL_ILLOPC:
                    return "Illegal opcode";
                case ILL_ILLOPN:
                    return "Illegal operand";
                case ILL_ILLADR:
                    return "Illegal addressing mode";
                case ILL_ILLTRP:
                    return "Illegal trap";
                case ILL_PRVOPC:
                    return "Privileged opcode";
                case ILL_PRVREG:
                    return "Privileged register";
                case ILL_COPROC:
                    return "Coprocessor error";
                case ILL_BADSTK:
                    return "Internal stack error";
                default:
                    return "Illegal operation";
            }
        case SIGFPE:
            switch (code) {
                case FPE_INTDIV:
                    return "Integer divide by zero";
                case FPE_INTOVF:
                    return "Integer overflow";
                case FPE_FLTDIV:
                    return "Floating-point divide by zero";
                case FPE_FLTOVF:
                    return "Floating-point overflow";
                case FPE_FLTUND:
                    return "Floating-point underflow";
                case FPE_FLTRES:
                    return "Floating-point inexact result";
                case FPE_FLTINV:
                    return "Invalid floating-point operation";
                case FPE_FLTSUB:
                    return "Subscript out of range";
                default:
                    return "Floating-point";
            }
        case SIGSEGV:
            switch (code) {
                case SEGV_MAPERR:
                    return "Address not mapped to object";
                case SEGV_ACCERR:
                    return "Invalid permissions for mapped object";
                default:
                    return "Segmentation violation";
            }
        case SIGBUS:
            switch (code) {
                case BUS_ADRALN:
                    return "Invalid address alignment";
                case BUS_ADRERR:
                    return "Nonexistent physical address";
                case BUS_OBJERR:
                    return "Object-specific hardware error";
                default:
                    return "Bus error";
            }
        case SIGTRAP:
            switch (code) {
                case TRAP_BRKPT:
                    return "Process breakpoint";
                case TRAP_TRACE:
                    return "Process trace trap";
                default:
                    return "Trap";
            }
        case SIGCHLD:
            switch (code) {
                case CLD_EXITED:
                    return "Child has exited";
                case CLD_KILLED:
                    return "Child has terminated abnormally and did not create a core file";
                case CLD_DUMPED:
                    return "Child has terminated abnormally and created a core file";
                case CLD_TRAPPED:
                    return "Traced child has trapped";
                case CLD_STOPPED:
                    return "Child has stopped";
                case CLD_CONTINUED:
                    return "Stopped child has continued";
                default:
                    return "Child";
            }
        case SIGPOLL:
            switch (code) {
                case POLL_IN:
                    return "Data input available";
                case POLL_OUT:
                    return "Output buffers available";
                case POLL_MSG:
                    return "Input message available";
                case POLL_ERR:
                    return "I/O error";
                case POLL_PRI:
                    return "High priority input available";
                case POLL_HUP:
                    return "Device disconnected";
                default:
                    return "Pool";
            }
        case SIGABRT:
            return "Process abort signal";
        case SIGALRM:
            return "Alarm clock";
        case SIGCONT:
            return "Continue executing, if stopped";
        case SIGHUP:
            return "Hangup";
        case SIGINT:
            return "Terminal interrupt signal";
        case SIGKILL:
            return "Kill";
        case SIGPIPE:
            return "Write on a pipe with no one to read it";
        case SIGQUIT:
            return "Terminal quit signal";
        case SIGSTOP:
            return "Stop executing";
        case SIGTERM:
            return "Termination signal";
        case SIGTSTP:
            return "Terminal stop signal";
        case SIGTTIN:
            return "Background process attempting read";
        case SIGTTOU:
            return "Background process attempting write";
        case SIGUSR1:
            return "User-defined signal 1";
        case SIGUSR2:
            return "User-defined signal 2";
        case SIGPROF:
            return "Profiling timer expired";
        case SIGSYS:
            return "Bad system call";
        case SIGVTALRM:
            return "Virtual timer expired";
        case SIGURG:
            return "High bandwidth data is available at a socket";
        case SIGXCPU:
            return "CPU time limit exceeded";
        case SIGXFSZ:
            return "File size limit exceeded";
        default:
            switch (code) {
                case SI_USER:
                    return "Signal sent by kill()";
                case SI_QUEUE:
                    return "Signal sent by the sigqueue()";
                case SI_TIMER:
                    return "Signal generated by expiration of a timer set by timer_settime()";
                case SI_ASYNCIO:
                    return "Signal generated by completion of an asynchronous I/O request";
                case SI_MESGQ:
                    return
                            "Signal generated by arrival of a message on an empty message queue";
                default:
                    return "Unknown signal";
            }
    }
}

/**
 * 获取线程名称
 * @param tid
 * @return
 */
static char *get_thread_name(pid_t tid) {

    if (tid <= 1) {
        return NULL;
    }
    char *path = (char *) calloc(1, 80);
    char *line = (char *) calloc(1, THREAD_NAME_LENGTH);

    snprintf(path, PATH_MAX, "proc/%d/comm", tid);
    FILE *commFile = NULL;
    if (commFile = fopen(path, "r")) {
        fgets(line, THREAD_NAME_LENGTH, commFile);
        fclose(commFile);
    }
    free(path);
    if (line) {
        int length = strlen(line);
        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
    }
    return line;
}


/**
 * 信号对应的处理函数
 * @param code    是sig信号对应值
 * @param siginfo
 * @param sc
 */
static void sig_handler(const int code, siginfo *siginfo, void *context) {


    LOGE("收到信号对应的code:%d, si_code:%d，错误信息：%s", code, siginfo->si_code,
         sig_desc(code, siginfo->si_code));
    //要返回的堆栈日志
    char log[1024];
    snprintf(log, sizeof(log), "signal %d （%s）", code, sig_desc(code, siginfo->si_code));
    append(stack_log, log);

    //开始解堆栈
    slow_backtrace();
    pid_t tid = gettid();
    pthread_t t = pthread_self();
    LOGE("pid_t = %d, pthread_self = %d", tid, t);
    //获取线程名称
    thread_name = get_thread_name(tid);


    //通知等待信号的线程
    pthread_mutex_lock(&mtx);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);

    //据pc的值，获取动态库的起始地址
    //据code找到之前的信号处理
    struct sigaction old_sig_act = p_sa_old[code];
    //调用之前的处理
    old_sig_act.sa_sigaction(code, siginfo, context);

}

//注册信号的处理
static int register_crash_handler() {
    //为信号处理函数注册一个栈
    stack_t stack;
    memset(&stack, 0, sizeof(stack));
    stack.ss_size = SIGSTKSZ;
    stack.ss_sp = malloc(stack.ss_size);
    stack.ss_flags = 0;
    if (stack.ss_sp == NULL || sigaltstack(&stack, NULL) != 0) {
        return ERROR;
    }
    //需要捕获的信号
    //定义信号对应的处理结构体sigaction
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = sig_handler;

    //分配保存之前的信号处理结构体的内存
    p_sa_old = static_cast<struct sigaction *>(calloc(sizeof(struct sigaction), SIG_NUMBER_MAX));

    for (int i = 0; sig_arr[i] != 0; i++) {
        int sig = sig_arr[i];
        if (sigaction(sig, &sa, &p_sa_old[sig]) != 0) {
            LOGE("信号处理注册失败");
            return ERROR;
        }
    }
    LOGE("信号处理注册成功");
    return SUCCESS;

}


/**
 * 在信号处理函数中回调Java方法总是失败，所以在新线程中回调Java方法
 * @param argv
 * @return
 */
void *dumpStack(void *argv) {
    //将当前线程attach到虚拟机
    JNIEnv *env;
    if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("当前线程AttachCurrentThread失败");
        return nullptr;
    }


    while (!b_stop) {
        //等待信号
        char *stack_temp;
        pthread_mutex_lock(&mtx);
        pthread_cond_wait(&cond, &mtx);
        LOGE("开始将堆栈拷贝到局部变量");
        stack_temp = static_cast<char *>(malloc(sizeof(stack_log)));
        strcpy(stack_temp, stack_log);
        pthread_mutex_unlock(&mtx);

        LOGE("开始将堆栈回调到Java层");
        //将堆栈发送到Java层
        jclass clz = env->GetObjectClass(g_obj);
        jmethodID jmethodId = env->GetStaticMethodID(clz, "onNativeLog", "(Ljava/lang/String;Ljava/lang/String;)V");
        jstring jstack = env->NewStringUTF(stack_temp);

        //释放资源
        free(stack_temp);
        if (thread_name == nullptr) {
            thread_name = "";
        }
        env->CallStaticVoidMethod(clz, jmethodId, jstack, env->NewStringUTF(thread_name));
        env->DeleteLocalRef(jstack);


    }

}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_crashsdk_MainActivity_stringFromJNI(JNIEnv *env, jobject thiz, jobject crash_log) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_jvm = vm;//获取一个全局的VM指针
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_crashsdk_MainActivity_initCrashSDK(JNIEnv *env, jobject thiz, jobject crash_log) {
    //创建一个全局的引用
    g_obj = env->NewGlobalRef(crash_log);
    //开启子线程，等待信号
    pthread_t tid;
    int ret = pthread_create(&tid, nullptr, dumpStack, nullptr);
    if (ret) {
        return ret;
    }
    //注册崩溃处理程序
    ret = register_crash_handler();

    //触发崩溃
    char *name = nullptr;
    strlen(name);
    return ret;
}