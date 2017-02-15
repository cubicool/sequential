#ifndef SEQUENTIAL_H
#define SEQUENTIAL_H 1

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
	#define SEQ_API __declspec(dllexport)

#else
	#define SEQ_API
#endif

#include <stdarg.h>
#include <stdint.h>

/* ======================================================================== Types, Constants, Enums
 * SEQ_VERSION
 * seq_t
 * seq_bool_t
 * seq_opt_t
 * seq_size_t
 * seq_args_t
 * seq_data_t
 * seq_get_t
 * seq_cb_add_t
 * seq_cb_remove_t
 * seq_cb_debug_t
 * ============================================================================================= */

#define SEQ_VERSION_MAJOR 0
#define SEQ_VERSION_MINOR 2
#define SEQ_VERSION_BUGFIX 0
#define SEQ_VERSION "0.2.0"

/* Most of the types used when interacting with Sequential are actually wrapped pointers. This kind
 * of API has always felt superior to me. */
typedef struct _seq_t* seq_t;
typedef struct _seq_iter_t* seq_iter_t;

typedef enum _seq_bool_t {
	SEQ_FALSE,
	SEQ_TRUE
} seq_bool_t;

/* Throughout Sequential, anytime a SEQ_* constant is expected seq_opt_t is used to manage it. */
typedef uint32_t seq_opt_t;

/* Sequential uses int64_t (rather than size_t) for its sizing and indexing API. This allows the
 * use of negative indices, indicating that the collection should be accessed/traversed in
 * reverse. */
typedef int32_t seq_size_t;

typedef va_list* seq_args_t;
typedef void* seq_data_t;

/* This structure is a special type returned by any of the "getter" routines within the Sequential
 * library. It includes not only the data component, but the handle by which that data is retrieved.
 * In most cases this is the numeric index of the node containing the data, but when using a SEQ_MAP
 * instance, this could be any arbitrary type. */
typedef struct _seq_get_t {
	seq_data_t data;

	union {
		seq_data_t key;
		seq_size_t index;
	} handle;
} seq_get_t;

#define SEQ_TYPE 0x11110000
#define SEQ_LIST (SEQ_TYPE | 0x0001)
#define SEQ_MAP (SEQ_TYPE | 0x0002)
#define SEQ_RING (SEQ_TYPE | 0x0003)
#define SEQ_QUEUE (SEQ_TYPE | 0x0004)
#define SEQ_STACK (SEQ_TYPE | 0x0005)
#define SEQ_ARRAY (SEQ_TYPE | 0x0006)
#define SEQ_TYPE_MAX SEQ_ARRAY

#define SEQ_ADD 0x22220000
#define SEQ_APPEND (SEQ_ADD | 0x0001)
#define SEQ_PREPEND (SEQ_ADD | 0x0002)
#define SEQ_BEFORE (SEQ_ADD | 0x0003)
#define SEQ_AFTER (SEQ_ADD | 0x0004)
#define SEQ_REPLACE (SEQ_ADD | 0x0005)
#define SEQ_KEYVAL (SEQ_ADD | 0x0006)
#define SEQ_SEND (SEQ_ADD | 0x0007)
#define SEQ_PUSH (SEQ_ADD | 0x0008)
#define SEQ_ADD_MAX SEQ_PUSH

#define SEQ_GET 0x33330000
#define SEQ_INDEX (SEQ_GET | 0x0001)
#define SEQ_KEY (SEQ_GET | 0x0002)
#define SEQ_RECV (SEQ_GET | 0x0003)
#define SEQ_POP (SEQ_GET | 0x0004)
#define SEQ_DATA (SEQ_GET | 0x0005)
#define SEQ_GET_MAX SEQ_DATA

#define SEQ_SET 0x44440000
#define SEQ_CB_ADD (SEQ_SET | 0x0001)
#define SEQ_CB_REMOVE (SEQ_SET | 0x0002)
#define SEQ_CB_REMOVE_FREE (SEQ_SET | 0x0003)
#define SEQ_CB_DEBUG (SEQ_SET | 0x0004)
#define SEQ_DEBUG_STDOUT (SEQ_SET | 0x0005)
#define SEQ_DEBUG_STDERR (SEQ_SET | 0x0006)
#define SEQ_DEBUG_FWRITE (SEQ_SET | 0x0007)
#define SEQ_DEBUG_PREFIX (SEQ_SET | 0x0008)
#define SEQ_DEBUG_POSTFIX (SEQ_SET | 0x0009)
#define SEQ_DEBUG_LEVEL (SEQ_SET | 0x000A)
#define SEQ_SET_MAX SEQ_DEBUG_LEVEL

#define SEQ_ITER 0x55550000
#define SEQ_READY (SEQ_ITER | 0x0001)
#define SEQ_ACTIVE (SEQ_ITER | 0x0002)
#define SEQ_STOP (SEQ_ITER | 0x0003)
#define SEQ_RANGE (SEQ_ITER | 0x0004)
#define SEQ_INC (SEQ_ITER | 0x0005)
#define SEQ_ITER_MAX SEQ_INC

#define SEQ_LEVEL 0x66660000
#define SEQ_INFO (SEQ_LEVEL | 0x0001)
#define SEQ_ERROR (SEQ_LEVEL | 0x0002)
#define SEQ_LEVEL_MAX SEQ_ERROR

/* The seq_cb_add_t type defines the signature of an optional callback that will be used internally
 * by the seq_t instance when seq_add() is called. It is passed the remainder of the argument list
 * occurring after thte SEQ_ADD constant; e.g., seq_add(seq, SEQ_APPEND, 1, 2, 3) would forward the
 * (1, 2, 3) portion of the va_list to the function.
 *
 * When present on a seq_t instance, this callback must return a seq_data_t value that will be bound
 * internally to the current element. */
typedef seq_data_t (*seq_cb_add_t)(seq_args_t args);

/* This optional callback is used when a seq_t instance removes an element. It is passed the same
 * seq_data_t value generated by the implementation's seq_add() function (which, in turn, will use
 * the value returned from the seq_cb_add_t callback, if set). */
typedef void (*seq_cb_remove_t)(seq_data_t data);

typedef void (*seq_cb_debug_t)(seq_opt_t level, const char* msg, seq_data_t data);

#define seq_arg(args, type) va_arg(*args, type)
#define seq_arg_size(args) va_arg(*args, seq_size_t)
#define seq_arg_data(args) va_arg(*args, seq_data_t)
#define seq_arg_opt(args) va_arg(*args, seq_opt_t)

/* ======================================================================================= Core API
 * seq_create
 * seq_destroy
 * seq_add
 * seq_remove
 * seq_get
 * seq_set
 * seq_type
 * seq_size
 * ============================================================================================= */

/* Creates a new, empty seq_t instance using the SEQ_LIST implementation. */
SEQ_API seq_t seq_create(seq_opt_t type);

/* Properly destroys all attached elements, and any implementation-specific data, of the passed-in
 * seq_t instance. */
SEQ_API void seq_destroy(seq_t seq);

/* The routine used (in conjuction with some seq_opt_t constant) to add data to the specified seq_t
 * instance. Depending on the implementation in use, this function may have many potential variants;
 * or just one. More information is can be found in the SEQ_ADD documentation. */
SEQ_API seq_bool_t seq_add(seq_t seq, ...);
SEQ_API seq_bool_t seq_vadd(seq_t seq, seq_args_t args);

/* Acts as the inverse of seq_add(), requiring implementation-specific usage to remove an existing
 * element of the seq_t instance. */
SEQ_API seq_bool_t seq_remove(seq_t seq, ...);
SEQ_API seq_bool_t seq_vremove(seq_t seq, seq_args_t args);

/* Retrieves the seq_data_t attached to the specified node. Just as with seq_add() and seq_remove(),
 * the internal instance implementation will determine the exact manner in which this routine is
 * used. */
SEQ_API seq_get_t seq_get(seq_t seq, ...);
SEQ_API seq_get_t seq_vget(seq_t seq, seq_args_t args);

/* This function provides a mechanism by which various behaviors of a seq_t instance can be enabled
 * and disabled. More information is available with the SEQ_SET documentation. */
SEQ_API seq_bool_t seq_set(seq_t seq, ...);
SEQ_API seq_bool_t seq_vset(seq_t seq, seq_args_t args);

/* Returns the SEQ_TYPE constants that this seq_t instance was created with. */
SEQ_API seq_opt_t seq_type(seq_t seq);

/* Returns the number of nodes attached to this instance. */
SEQ_API seq_size_t seq_size(seq_t seq);

/* ================================================================================== Iteration API
 * seq_iter_create
 * seq_iter_destroy
 * seq_iter_get
 * seq_iter_set
 * seq_iterate
 * ============================================================================================= */

SEQ_API seq_iter_t seq_iter_create(seq_t seq, ...);
SEQ_API seq_iter_t seq_iter_vcreate(seq_t seq, seq_args_t args);

SEQ_API void seq_iter_destroy(seq_iter_t iter);

SEQ_API seq_get_t seq_iter_get(seq_iter_t iter, ...);
SEQ_API seq_get_t seq_iter_vget(seq_iter_t iter, seq_args_t args);

SEQ_API seq_bool_t seq_iter_set(seq_iter_t iter, ...);
SEQ_API seq_bool_t seq_iter_vset(seq_iter_t iter, seq_args_t args);

SEQ_API seq_bool_t seq_iterate(seq_iter_t iter);

/* ============================================================================== Miscellaneous API
 * seq_opt_str
 * seq_save
 * seq_restore
 * ...
 * ============================================================================================= */

/* Converts the given constant--that is, one of the many SEQ_* defines--and returns its string
 * representation, omitting the leading "SEQ_" prefix. */
SEQ_API const char* seq_opt_str(seq_opt_t opt);

#ifdef __cplusplus
}
#endif

#endif

