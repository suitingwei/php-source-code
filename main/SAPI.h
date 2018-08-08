/* $Id$ */

#ifndef SAPI_H
#define SAPI_H

#include "php.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_llist.h"
#include "zend_operators.h"
#ifdef PHP_WIN32
#include "win32/php_stdint.h"
#endif
#include <sys/stat.h>

#define SAPI_OPTION_NO_CHDIR 1
#define SAPI_POST_BLOCK_SIZE 0x4000

#if defined(__GNUC__) && __GNUC__ >= 4
#	define SAPI_API __attribute__ ((visibility("default")))
#else
#	define SAPI_API
#endif

#undef shutdown

// http header 结构体,这是保存的某一个 header
// connection: keep-alive 或者 Date: Tue, 07 Aug 2018 08:33:30 GMT 等
typedef struct {
	char *header;
	size_t header_len;
} sapi_header_struct;

//headers 的列表
typedef struct {
	//todo headers，应该是一堆spi_header_struct才对
	zend_llist headers;

	//http 返回的 status-code
	int http_response_code;

	//默认的http-content-type
	unsigned char send_default_content_type;

	//mine-type
	char *mimetype;

	//这个不知道是啥
	char *http_status_line;
} sapi_headers_struct;

/**
 * http post 处理器
 * ----------------------------
 */
typedef struct _sapi_post_entry {
	//http-content-type
	char *content_type;

	//content的长度
	uint32_t content_type_len;

	//读取post数据的handler
	void (*post_reader)(void);

	//处理post数据的handler
	void (*post_handler)(char *content_type_dup, void *arg);
} sapi_post_entry;


/**
 * SAPI核心结构体。
 * 表示了正在运行的 SAPI：fpm,cli
 */
typedef struct _sapi_module_struct {
    //SAPI 名字
	char *name;

	//SAPI 长名字
	char *pretty_name;

	/**
	 * 启动函数。
	 * ----------------------------------------------------------
	 * 这是一个会返回int的函数,函数的参数是： _sapi_module_struct
	 * 这里配置的是函数指针。所以需要(*startUp)这么写法。适应就好。
	 */
	int (*startup)(struct _sapi_module_struct *sapi_module);

	//关闭函数，和上述启动函数一致。
	int (*shutdown)(struct _sapi_module_struct *sapi_module);

	//激活函数，跟上述启动函数无异。不过不需要参数而已。
	int (*activate)(void);

	//关闭激活函数。
	int (*deactivate)(void);

	/**
	 * 输出某些字符串。
	 */
	size_t (*ub_write)(const char *str, size_t str_length);

    /**
     * flush
     */
	void (*flush)(void *server_context);
	
	zend_stat_t *(*get_stat)(void);

	//获取env配置
	char *(*getenv)(char *name, size_t name_len);

	//处理sapi_error
	void (*sapi_error)(int type, const char *error_msg, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);

	int (*header_handler)(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers);
	int (*send_headers)(sapi_headers_struct *sapi_headers);
	void (*send_header)(sapi_header_struct *sapi_header, void *server_context);

	size_t (*read_post)(char *buffer, size_t count_bytes);
	char *(*read_cookies)(void);

	void (*register_server_variables)(zval *track_vars_array);
	void (*log_message)(char *message, int syslog_type_int);
	double (*get_request_time)(void);
	void (*terminate_process)(void);

	char *php_ini_path_override;

	void (*default_post_reader)(void);
	void (*treat_data)(int arg, char *str, zval *destArray);
	char *executable_location;

	int php_ini_ignore;
	int php_ini_ignore_cwd; /* don't look for php.ini in the current directory */

	int (*get_fd)(int *fd);

	int (*force_http_10)(void);

	int (*get_target_uid)(uid_t *);
	int (*get_target_gid)(gid_t *);

	unsigned int (*input_filter)(int arg, char *var, char **val, size_t val_len, size_t *new_val_len);

 	/**
 	 * 初始化ini 配置的。
 	 */
	void (*ini_defaults)(HashTable *configuration_hash);
	int phpinfo_as_text;

	char *ini_entries;
	const zend_function_entry *additional_functions;
	unsigned int (*input_filter_init)(void);
} sapi_module_struct;

/**
 * 请求信息结构体，一般来说就是 http-request。
 * 不知道对于cli 模式来说这个变量保存的是: argc, argv
 * 
 * ---------------------------------------------------------------------
 * Some values in this structure needs to be filled in before
 * calling sapi_activate(). We WILL change the `char *' entries,
 * so make sure that you allocate a separate buffer for them
 * and that you free them after sapi_deactivate().
 */
typedef struct {

	//请求方法，POST, GET, DELETE
	const char *request_method; 

	//query string, `?a=1&b=2&c=3`,这后边的一串字符串
	char *query_string;

	//cookie 信息
	char *cookie_data;

	//todo 请求的 content-length，这个应该是字节长度? 
	zend_long content_length;

	//todo 这个变量暂时不知道是什么? 
	char *path_translated;

	//请求的地址 protocol://host
	char *request_uri;

	/* Do not use request_body directly, but the php://input stream wrapper instead */
	struct _php_stream *request_body;

	//http-content-type, application/json, text/html等
	const char *content_type;

	//是否只有header, unsigned char, 一个字节的无符号， 8bit, 也就是从 0 ~ 2^8-1 : 0~255
	zend_bool headers_only;

	//是否没有 header
	zend_bool no_headers;

	//header 是否读取了? 或者是否只读？
	zend_bool headers_read;

	//post 的 entry? 
	sapi_post_entry *post_entry;

	//todo 不知道做啥的
	char *content_type_dup;

	/* for HTTP authentication */
	char *auth_user;
	char *auth_password;
	char *auth_digest;

	/* this is necessary for the CGI SAPI module */
	char *argv0;

	char *current_user;
	int current_user_length;

	/*cli 模式的输入参数*/
	int argc;
	char **argv;
	int proto_num;
} sapi_request_info;


/**
 * 全局的sapi_globals结构体. 猜测也是保存一些全局的信息。
 * -----------------------------------------------------------------------
 * 主要是请求信息，post 数据，请求时间，请求数据大小，mime-types,content-type
 * 一大堆数据。还有俩回调，暂时不知道是什么。
 */
typedef struct _sapi_globals_struct {

	// 服务器上下文？ 
	void *server_context;

	//请求信息 @see main/sapi.h/sapi_request_info
	sapi_request_info request_info;

    //请求的 headers
	sapi_headers_struct sapi_headers;

	//已经读取的 post数据字节
	int64_t read_post_bytes;

	//todo 读取的 post? 
	unsigned char post_read;

	//发送的 headers ?
	unsigned char headers_sent;

	/**
	 * 全局的统计数据,这里的结构体对于linux来说是: <sys/stat.h>
	 * 后来查了一下，这个头文件定义的主要是对于文件状态的一些结构定义，比如 ls - l 
	 * 命令，其实就是使用了这个结构体。大体结构如下：
	 * ------------------------------------------------------------------------
	 * struct stat {
     *    mode_t     st_mode;       //文件对应的模式，文件，目录等
     *    ino_t      st_ino;       //inode节点号
     *    dev_t      st_dev;        //设备号码
     *    dev_t      st_rdev;       //特殊设备号码
     *    nlink_t    st_nlink;      //文件的连接数
     *    uid_t      st_uid;        //文件所有者
     *    gid_t      st_gid;        //文件所有者对应的组
     *    off_t      st_size;       //普通文件，对应的文件字节数
     *    time_t     st_atime;      //文件最后被访问的时间
     *    time_t     st_mtime;      //文件内容最后被修改的时间
     *    time_t     st_ctime;      //文件状态改变时间
     *    blksize_t st_blksize;    //文件内容对应的块大小
     *    blkcnt_t   st_blocks;     //伟建内容对应的块数量
      };
    */
	zend_stat_t global_stat;

	//默认的mime-types
	char *default_mimetype;

	//默认的字符集
	char *default_charset;

	//todo rfc 1867定义的上传文件结构,继续了解rfc1867
	HashTable *rfc1867_uploaded_files;

	//post 的最大大小
	zend_long post_max_size;

	//不知道是啥选项
	int options;

	//sapi 是否启动成功
	zend_bool sapi_started;

	//全局的启动时间
	double global_request_time;

	//已知的的content-types
	HashTable known_post_content_types;

	//todo 回调？
	zval callback_func;

	//todo 不知道是啥？
	zend_fcall_info_cache fci_cache;
} sapi_globals_struct;


/*
 * Always specify a sapi_header_line this way:
 *
 * sapi_header_line ctr = {0};
 */
typedef struct {
	char *line; /* If you allocated this, you need to free it yourself */
	size_t line_len;
	zend_long response_code; /* long due to zend_parse_parameters compatibility */
} sapi_header_line;


/**
 * HEADER 操作的枚举值
 */
typedef enum {					/* Parameter: 			*/
	SAPI_HEADER_REPLACE,		/* sapi_header_line* 	*/
	SAPI_HEADER_ADD,			/* sapi_header_line* 	*/
	SAPI_HEADER_DELETE,			/* sapi_header_line* 	*/
	SAPI_HEADER_DELETE_ALL,		/* void					*/
	SAPI_HEADER_SET_STATUS		/* int 					*/
} sapi_header_op_enum;

/*---------------------------全局变量、全局函数声明------------------------------*/

/**
 * todo 这个函数很重要，各种地方都在使用。一定要看明白。
 */
#define SG(v) (sapi_globals.v)

#define sapi_add_header(a, b, c) sapi_add_header_ex((a),(b),(c),1)

#define SAPI_HEADER_ADD			(1<<0)

#define SAPI_HEADER_SENT_SUCCESSFULLY	1
#define SAPI_HEADER_DO_SEND				2
#define SAPI_HEADER_SEND_FAILED			3

#define SAPI_DEFAULT_MIMETYPE		"text/html"
#define SAPI_DEFAULT_CHARSET		PHP_DEFAULT_CHARSET
#define SAPI_PHP_VERSION_HEADER		"X-Powered-By: PHP/" PHP_VERSION

#define SAPI_POST_READER_FUNC(post_reader) void post_reader(void)
#define SAPI_POST_HANDLER_FUNC(post_handler) void post_handler(char *content_type_dup, void *arg)

#define SAPI_TREAT_DATA_FUNC(treat_data) void treat_data(int arg, char *str, zval* destArray)
#define SAPI_INPUT_FILTER_FUNC(input_filter) unsigned int input_filter(int arg, char *var, char **val, size_t val_len, size_t *new_val_len)

// 声明了一个 sapi_module_struct 的全局变量
// 作者说这个是真的全局变量
extern SAPI_API sapi_module_struct sapi_module; 

//声明了 sapi_globals_struct,保存全局信息
extern SAPI_API sapi_globals_struct sapi_globals;


SAPI_API void sapi_startup(sapi_module_struct *sf);
SAPI_API void sapi_shutdown(void);
SAPI_API void sapi_activate(void);
SAPI_API void sapi_deactivate(void);
SAPI_API void sapi_initialize_empty_request(void);
SAPI_API int sapi_header_op(sapi_header_op_enum op, void *arg);
/* Deprecated functions. Use sapi_header_op instead. */
SAPI_API int sapi_add_header_ex(char *header_line, size_t header_line_len, zend_bool duplicate, zend_bool replace);
SAPI_API int sapi_send_headers(void);
SAPI_API void sapi_free_header(sapi_header_struct *sapi_header);
SAPI_API void sapi_handle_post(void *arg);
SAPI_API size_t sapi_read_post_block(char *buffer, size_t buflen);
SAPI_API int sapi_register_post_entries(sapi_post_entry *post_entry);
SAPI_API int sapi_register_post_entry(sapi_post_entry *post_entry);
SAPI_API void sapi_unregister_post_entry(sapi_post_entry *post_entry);
SAPI_API int sapi_register_default_post_reader(void (*default_post_reader)(void));
SAPI_API int sapi_register_treat_data(void (*treat_data)(int arg, char *str, zval *destArray));
SAPI_API int sapi_register_input_filter(unsigned int (*input_filter)(int arg, char *var, char **val, size_t val_len, size_t *new_val_len), unsigned int (*input_filter_init)(void));
SAPI_API int sapi_flush(void);
SAPI_API zend_stat_t *sapi_get_stat(void);
SAPI_API char *sapi_getenv(char *name, size_t name_len);
SAPI_API char *sapi_get_default_content_type(void);
SAPI_API void sapi_get_default_content_type_header(sapi_header_struct *default_header);
SAPI_API size_t sapi_apply_default_charset(char **mimetype, size_t len);
SAPI_API void sapi_activate_headers_only(void);
SAPI_API int sapi_get_fd(int *fd);
SAPI_API int sapi_force_http_10(void);
SAPI_API int sapi_get_target_uid(uid_t *);
SAPI_API int sapi_get_target_gid(gid_t *);
SAPI_API double sapi_get_request_time(void);
SAPI_API void sapi_terminate_process(void);
SAPI_API SAPI_POST_READER_FUNC(sapi_read_standard_form_data);
SAPI_API SAPI_POST_READER_FUNC(php_default_post_reader);
SAPI_API SAPI_TREAT_DATA_FUNC(php_default_treat_data);
SAPI_API SAPI_INPUT_FILTER_FUNC(php_default_input_filter);

#define STANDARD_SAPI_MODULE_PROPERTIES \
	NULL, /* php_ini_path_override   */ \
	NULL, /* default_post_reader     */ \
	NULL, /* treat_data              */ \
	NULL, /* executable_location     */ \
	0,    /* php_ini_ignore          */ \
	0,    /* php_ini_ignore_cwd      */ \
	NULL, /* get_fd                  */ \
	NULL, /* force_http_10           */ \
	NULL, /* get_target_uid          */ \
	NULL, /* get_target_gid          */ \
	NULL, /* input_filter            */ \
	NULL, /* ini_defaults            */ \
	0,    /* phpinfo_as_text;        */ \
	NULL, /* ini_entries;            */ \
	NULL, /* additional_functions    */ \
	NULL  /* input_filter_init       */

#endif /* SAPI_H */

