#include "seq-api.h"

#include <string.h>

#define seq_args_wrap(func, start, ret) \
	seq_args_t args; \
	seq_args_start(args, start); \
	ret = seq_##func(start, args); \
	seq_args_end(args)

#define seq_args_wrap_debug(seq, level, start) \
	seq_args_t args; \
	seq_args_start(args, start); \
	seq_debug(seq, level, start, args); \
	seq_args_end(args)

/* ============================================================================================= */
static void seq_cb_remove_free(seq_data_t data) {
	free(data);
}

static void seq_cb_debug_stdout(seq_opt_t level, const char* message, seq_data_t data) {
	fprintf(stdout, message);
}

static void seq_cb_debug_stderr(seq_opt_t level, const char* message, seq_data_t data) {
	fprintf(stderr, message);
}

static void seq_cb_debug_fwrite(seq_opt_t level, const char* message, seq_data_t data) {
	fprintf((FILE*)(data), message);
}

/* ============================================================================================= */
/* NOTE: We can't call seq_trace() here because we do not have a valid context. */
seq_t seq_create(seq_opt_t type) {
	seq_t seq = NULL;

	if(seq_opt(type, SEQ_TYPE) && type == SEQ_LIST) {
		seq = seq_malloc(seq_t);

		if(!seq) return NULL;

		seq->set.debug.prefix = "";
		seq->set.debug.postfix = "";
		seq->set.debug.level = SEQ_ERROR;
		seq->set.debug.depth = 0;

		seq_impl_list()->create(seq);
	}

	return seq;
}

void seq_destroy(seq_t seq) {
	seq->impl->destroy(seq);

	free(seq);
}

seq_bool_t seq_add(seq_t seq, ...) {
	seq_bool_t r = SEQ_FALSE;

	seq_args_wrap(vadd, seq, r);

	return r;
}

seq_bool_t seq_vadd(seq_t seq, seq_args_t args) {
	if(seq->impl->add(seq, args)) {
		seq->size++;

		return SEQ_TRUE;
	}

	seq_error(seq, "impl->add() callback failed");

	return SEQ_FALSE;
}

seq_bool_t seq_remove(seq_t seq, ...) {
	seq_bool_t r = SEQ_FALSE;

	seq_args_wrap(vremove, seq, r);

	return r;
}

seq_bool_t seq_vremove(seq_t seq, seq_args_t args) {
	if(seq->impl->remove(seq, args)) {
		seq->size--;

		return SEQ_TRUE;
	}

	return SEQ_FALSE;
}

seq_data_t seq_get(seq_t seq, ...) {
	seq_data_t data = NULL;

	seq_args_wrap(vget, seq, data);

	return data;
}

seq_data_t seq_vget(seq_t seq, seq_args_t args) {
	return seq->impl->get(seq, args);
}

seq_bool_t seq_set(seq_t seq, ...) {
	seq_bool_t r = SEQ_FALSE;

	seq_args_wrap(vset, seq, r);

	return r;
}

seq_bool_t seq_vset(seq_t seq, seq_args_t args) {
	seq_opt_t set = seq_arg_opt(args);

	if(!seq_opt(set, SEQ_SET)) return SEQ_FALSE;

	if(set == SEQ_CB_ADD) {
		seq_cb_add_t add = seq_arg(args, seq_cb_add_t);

		if(!add) return SEQ_FALSE;

		seq->set.add = add;
	}

	else if(set == SEQ_CB_REMOVE) {
		seq_cb_remove_t remove = seq_arg(args, seq_cb_remove_t);

		if(!remove) return SEQ_FALSE;

		seq->set.remove = remove;
	}

	else if(set == SEQ_CB_DEBUG) {
		seq->set.debug.debug = seq_arg(args, seq_cb_debug_t);
		seq->set.debug.data = seq_arg(args, FILE*);
	}

	else if(set == SEQ_CB_REMOVE_FREE) seq->set.remove = seq_cb_remove_free;

	else if(set == SEQ_DEBUG_STDOUT) seq->set.debug.debug = seq_cb_debug_stdout;

	else if(set == SEQ_DEBUG_STDERR) seq->set.debug.debug = seq_cb_debug_stderr;

	else if(set == SEQ_DEBUG_FWRITE) {
		seq->set.debug.debug = seq_cb_debug_fwrite;
		seq->set.debug.data = seq_arg(args, FILE*);
	}

	else if(set == SEQ_DEBUG_PREFIX) seq->set.debug.prefix = seq_arg(args, const char*);

	else if(set == SEQ_DEBUG_POSTFIX) seq->set.debug.postfix = seq_arg(args, const char*);

	/* TODO: Call impl->set() */
	else {
		return SEQ_FALSE;
	}

	return SEQ_TRUE;
}

seq_opt_t seq_type(seq_t seq) {
	return seq->type;
}

seq_size_t seq_size(seq_t seq) {
	return seq->size;
}

/* ============================================================================================= */
seq_iter_t seq_iter_create(seq_t seq, ...) {
	seq_iter_t iter = NULL;

	seq_args_wrap(iter_vcreate, seq, iter);

	return iter;
}

seq_iter_t seq_iter_vcreate(seq_t seq, seq_args_t args) {
	seq_iter_t iter = seq_malloc(seq_iter_t);

	if(!iter) return NULL;

	iter->seq = seq;
	iter->state = SEQ_READY;

	seq->impl->iter.create(iter, args);

	return iter;
}

void seq_iter_destroy(seq_iter_t iter) {
	iter->seq->impl->iter.destroy(iter);

	free(iter->data);
	free(iter);
}

seq_data_t seq_iter_get(seq_iter_t iter, ...) {
	seq_data_t data = NULL;

	seq_args_wrap(iter_vget, iter, data);

	return data;
}

seq_data_t seq_iter_vget(seq_iter_t iter, seq_args_t args) {
	return iter->seq->impl->iter.get(iter, args);
}

seq_bool_t seq_iter_set(seq_iter_t iter, ...) {
	seq_bool_t r = SEQ_FALSE;

	seq_args_wrap(iter_vset, iter, r);

	return r;
}

seq_bool_t seq_iter_vset(seq_iter_t iter, seq_args_t args) {
	return iter->seq->impl->iter.set(iter, args);
}

seq_bool_t seq_iterate(seq_iter_t iter) {
	return iter->seq->impl->iter.iterate(iter);
}

/* ============================================================================================= */
static const char* seq_level_str[] = { "TRACE", "INFO", "ERROR" };

static void seq_debug(seq_t seq, seq_opt_t level, const char* fmt, seq_args_t args) {
	if(
		seq_opt(seq->set.debug.level, SEQ_LEVEL) &&
		seq->set.debug.debug != NULL &&
		seq_opt_val(level) <= seq_opt_val(seq->set.debug.level)
	) {
		char format[512];
		char result[1024];
		seq_size_t index = 0;

		/* if(seq->set.debug.depth > 0) {
			for(index = 0; index < seq->set.debug.depth; index++) result[index] = '.';
		} */

		sprintf(
			format + index,
			"%s[%s] %s%s (depth=%d)\n",
			seq->set.debug.prefix,
			seq_level_str[seq_opt_val(level) - 1],
			fmt,
			seq->set.debug.postfix,
			seq->set.debug.depth
		);

		vsprintf(result, format, args);

		seq->set.debug.debug(level, result, seq->set.debug.data);
	}
}

void seq_trace_begin(seq_t seq, const char* fmt, ...) {
	seq_args_wrap_debug(seq, SEQ_TRACE, fmt);

	seq->set.debug.depth++;
}

void seq_trace_end(seq_t seq, const char* fmt, ...) {
	seq->set.debug.depth--;

	{ seq_args_wrap_debug(seq, SEQ_TRACE, fmt); }
}

void seq_info(seq_t seq, const char* fmt, ...) {
	seq_args_wrap_debug(seq, SEQ_INFO, fmt);
}

void seq_error(seq_t seq, const char* fmt, ...) {
	seq_args_wrap_debug(seq, SEQ_ERROR, fmt);
}

