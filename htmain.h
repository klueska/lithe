/**
 * A framework or library that uses hard threads but does not want to
 * expose the hard threads interface should include this file in its
 * external header files.
 *
 * For now, you can use the storage specifier 'htls' in order to
 * request per hard thread local storage.
*/

#ifndef HT_MAIN_H
#define HT_MAIN_H

/**
 * Use the storage specifier 'htls' to indicate that the data should
 * be allocated per hard thread local storage.
 */
#define htls __thread


#endif // HT_MAIN_H
