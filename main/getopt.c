/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "php_getopt.h"

#define OPTERRCOLON (1)
#define OPTERRNF (2)
#define OPTERRARG (3)

/**
 * 输出 php 命令行的参数错误
 *
 */
static int php_opt_error(int argc, char * const *argv, int oint, int optchr, int err, int show_err) /* {{{ */
{
	if (show_err)
	{
		fprintf(stderr, "Error in argument %d, char %d: ", oint, optchr+1);
		switch(err)
		{
		case OPTERRCOLON:
			fprintf(stderr, ": in flags\n");
			break;
		case OPTERRNF:
			fprintf(stderr, "option not found %c\n", argv[oint][optchr]);
			break;
		case OPTERRARG:
			fprintf(stderr, "no argument for option %c\n", argv[oint][optchr]);
			break;
		default:
			fprintf(stderr, "unknown\n");
			break;
		}
	}
	return('?');
}
/* }}} */

PHPAPI int php_optidx = -1;

/**
 * 解析参数。
 * ------------------------------------------------------------------
 * 1. 从 cli/php_cli.c 中看过来的，解析命令行参数。 2018-08-06
 *
 * @parm int argc 				 参数数目
 * @parm const char* *argv 		 参数数组
 * @parm const opt_struct opts[] 可选参数数组
 * @parm char **optarg  		 可选参数
 * @parm int  *optind  			 参数索引?
 * @parm int  show_err  		 是否展示错误
 * @parm int  arg_start 		 参数开始索引
 */
PHPAPI int php_getopt(int argc, char* const *argv, const opt_struct opts[], char **optarg, int *optind, int show_err, int arg_start) /* {{{ */
{
	static int optchr = 0;
	static int dash = 0; /* have already seen the - */

	//保存opt arguments,这个变量是静态的。所以他会保存上一次的结果的哦
	static char **prev_optarg = NULL;

    //参数行的索引
	php_optidx = -1;

	//如果当前参数和上一次的参数不同，重置两个标志位。
	if(prev_optarg && prev_optarg != optarg) {
		/* reset the state */
		optchr = 0;
		dash = 0;
	}

	//重新赋值参数
	prev_optarg = optarg;

	//如果正在处理的下标索引已经超过了参数的总和，那肯定 gg 了
	if (*optind >= argc) {
		return(EOF);
	}

	//是否出现过中划线
	if (!dash) {
		//如果某一个参数的开头不是中划线开头，那么就是错误的。
		//比如： php -v; php -h
		if ((argv[*optind][0] !=  '-')) {
			return(EOF);
		} 

		//如果中划线后面没有参数，报错呗。 比如 `php -`,这种肯定要报错的
		if (!argv[*optind][1]) {
			return(EOF);
		}
	}
	
	/**
	 * 判断长短参数
	 * --------------------------------------------------------------
	 * cli 长参数模式: php --ini, php --rc
	 * cli 短参数模式: php -v, php -h
	 * 
	 * @author ufoddd001@gmail.com
	 * @date 2018-08-07
	 */
	if ((argv[*optind][0] == '-') && (argv[*optind][1] == '-')) {
		const char *pos;
		size_t arg_end = strlen(argv[*optind])-1;

		/* '--' indicates end of args if not followed by a known long option name */
		if (argv[*optind][2] == '\0') {
			(*optind)++;
			return(EOF);
		}

		arg_start = 2;

		/* Check for <arg>=<val> */
		if ((pos = php_memnstr(&argv[*optind][arg_start], "=", 1, argv[*optind]+arg_end)) != NULL) {
			arg_end = pos-&argv[*optind][arg_start];
			arg_start++;
		} else {
			arg_end--;
		}

		while (1) {
			php_optidx++;
			if (opts[php_optidx].opt_char == '-') {
				(*optind)++;
				return(php_opt_error(argc, argv, *optind-1, optchr, OPTERRARG, show_err));
			} else if (opts[php_optidx].opt_name && !strncmp(&argv[*optind][2], opts[php_optidx].opt_name, arg_end) && arg_end == strlen(opts[php_optidx].opt_name)) {
				break;
			}
		}

		optchr = 0;
		dash = 0;
		arg_start += (int)strlen(opts[php_optidx].opt_name);
	} 

    // 短参数模式
	else {
		if (!dash) {
			dash = 1;
			optchr = 1;
		}
		/* Check if the guy tries to do a -: kind of flag */
		if (argv[*optind][optchr] == ':') {
			dash = 0;
			(*optind)++;
			return (php_opt_error(argc, argv, *optind-1, optchr, OPTERRCOLON, show_err));
		}
		arg_start = 1 + optchr;
	}
	if (php_optidx < 0) {
		while (1) {
			php_optidx++;
			if (opts[php_optidx].opt_char == '-') {
				int errind = *optind;
				int errchr = optchr;

				if (!argv[*optind][optchr+1]) {
					dash = 0;
					(*optind)++;
				} else {
					optchr++;
					arg_start++;
				}
				return(php_opt_error(argc, argv, errind, errchr, OPTERRNF, show_err));
			} else if (argv[*optind][optchr] == opts[php_optidx].opt_char) {
				break;
			}
		}
	}
	if (opts[php_optidx].need_param) {
		/* Check for cases where the value of the argument
		is in the form -<arg> <val>, -<arg>=<varl> or -<arg><val> */
		dash = 0;
		if (!argv[*optind][arg_start]) {
			(*optind)++;
			if (*optind == argc) {
				/* Was the value required or is it optional? */
				if (opts[php_optidx].need_param == 1) {
					return(php_opt_error(argc, argv, *optind-1, optchr, OPTERRARG, show_err));
				}
			/* Optional value is not supported with -<arg> <val> style */
			} else if (opts[php_optidx].need_param == 1) {
				*optarg = argv[(*optind)++];
 			}
		} else if (argv[*optind][arg_start] == '=') {
			arg_start++;
			*optarg = &argv[*optind][arg_start];
			(*optind)++;
		} else {
			*optarg = &argv[*optind][arg_start];
			(*optind)++;
		}
		return opts[php_optidx].opt_char;
	} else {
		/* multiple options specified as one (exclude long opts) */
		if (arg_start >= 2 && !((argv[*optind][0] == '-') && (argv[*optind][1] == '-'))) {
			if (!argv[*optind][optchr+1])
			{
				dash = 0;
				(*optind)++;
			} else {
				optchr++;
			}
		} else {
			(*optind)++;
		}
		return opts[php_optidx].opt_char;
	}
	assert(0);
	return(0);	/* never reached */
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
