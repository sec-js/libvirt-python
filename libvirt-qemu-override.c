/*
 * libvir.c: this modules implements the main part of the glue of the
 *           libvir library and the Python interpreter. It provides the
 *           entry points where an automatically generated stub is
 *           unpractical
 *
 * Copyright (C) 2011-2019 Red Hat, Inc.
 *
 * Daniel Veillard <veillard@redhat.com>
 */

/* Horrible kludge to work around even more horrible name-space pollution
   via Python.h.  That file includes /usr/include/python3.x/pyconfig*.h,
   which has over 180 autoconf-style HAVE_* definitions.  Shame on them.  */
#undef HAVE_PTHREAD_H

#include <stdio.h>
#include <Python.h>
#include <libvirt/libvirt-qemu.h>
#include <libvirt/virterror.h>
#include "typewrappers.h"
#include "libvirt-utils.h"
#include "libvirt-qemu.h"
#ifndef __CYGWIN__
# include <fcntl.h>
#endif

#if 0
# define DEBUG_ERROR 1
#endif

#if DEBUG_ERROR
# define DEBUG(fmt, ...)            \
    printf(fmt, __VA_ARGS__)
#else
# define DEBUG(fmt, ...)            \
    while (0) {printf(fmt, __VA_ARGS__);}
#endif

/*******************************************
 * Helper functions to avoid importing modules
 * for every callback
 *******************************************/
#if LIBVIR_CHECK_VERSION(1, 2, 3)
static PyObject *libvirt_qemu_module;
static PyObject *libvirt_qemu_dict;

static PyObject *
getLibvirtQemuModuleObject(void)
{
    if (libvirt_qemu_module)
        return libvirt_qemu_module;

    // PyImport_ImportModule returns a new reference
    /* Bogus (char *) cast for RHEL-5 python API brokenness */
    libvirt_qemu_module = PyImport_ImportModule((char *)"libvirt_qemu");
    if (!libvirt_qemu_module) {
        DEBUG("%s Error importing libvirt_qemu module\n", __FUNCTION__);
        PyErr_Print();
        return NULL;
    }

    return libvirt_qemu_module;
}

static PyObject *
getLibvirtQemuDictObject(void)
{
    if (libvirt_qemu_dict)
        return libvirt_qemu_dict;

    // PyModule_GetDict returns a borrowed reference
    libvirt_qemu_dict = PyModule_GetDict(getLibvirtQemuModuleObject());
    if (!libvirt_qemu_dict) {
        DEBUG("%s Error importing libvirt_qemu dictionary\n", __FUNCTION__);
        PyErr_Print();
        return NULL;
    }

    Py_INCREF(libvirt_qemu_dict);
    return libvirt_qemu_dict;
}


static PyObject *
libvirt_qemu_lookupPythonFunc(const char *funcname)
{
    PyObject *python_cb;

    /* Lookup the python callback */
    python_cb = PyDict_GetItemString(getLibvirtQemuDictObject(), funcname);

    if (!python_cb) {
        DEBUG("%s: Error finding %s\n", __FUNCTION__, funcname);
        PyErr_Print();
        PyErr_Clear();
        return NULL;
    }

    if (!PyCallable_Check(python_cb)) {
        DEBUG("%s: %s is not callable\n", __FUNCTION__, funcname);
        return NULL;
    }

    return python_cb;
}
#endif /* LIBVIR_CHECK_VERSION(1, 2, 3) */


/************************************************************************
 *									*
 *		Statistics						*
 *									*
 ************************************************************************/

static PyObject *
libvirt_qemu_virDomainQemuMonitorCommand(PyObject *self ATTRIBUTE_UNUSED,
                                         PyObject *args)
{
    PyObject *py_retval;
    char *result = NULL;
    virDomainPtr domain;
    PyObject *pyobj_domain;
    unsigned int flags;
    char *cmd;
    int c_retval;

    if (!PyArg_ParseTuple(args, (char *)"OzI:virDomainQemuMonitorCommand",
                          &pyobj_domain, &cmd, &flags))
        return NULL;
    domain = (virDomainPtr) PyvirDomain_Get(pyobj_domain);

    if (domain == NULL)
        return VIR_PY_NONE;
    LIBVIRT_BEGIN_ALLOW_THREADS;
    c_retval = virDomainQemuMonitorCommand(domain, cmd, &result, flags);
    LIBVIRT_END_ALLOW_THREADS;

    if (c_retval < 0)
        return VIR_PY_NONE;

    py_retval = libvirt_constcharPtrWrap(result);
    VIR_FREE(result);
    return py_retval;
}

#if LIBVIR_CHECK_VERSION(0, 10, 0)
static PyObject *
libvirt_qemu_virDomainQemuAgentCommand(PyObject *self ATTRIBUTE_UNUSED,
                                       PyObject *args)
{
    PyObject *py_retval;
    char *result = NULL;
    virDomainPtr domain;
    PyObject *pyobj_domain;
    int timeout;
    unsigned int flags;
    char *cmd;

    if (!PyArg_ParseTuple(args, (char *)"OziI:virDomainQemuAgentCommand",
                          &pyobj_domain, &cmd, &timeout, &flags))
        return NULL;
    domain = (virDomainPtr) PyvirDomain_Get(pyobj_domain);

    if (domain == NULL)
        return VIR_PY_NONE;
    LIBVIRT_BEGIN_ALLOW_THREADS;
    result = virDomainQemuAgentCommand(domain, cmd, timeout, flags);
    LIBVIRT_END_ALLOW_THREADS;

    if (!result)
        return VIR_PY_NONE;

    py_retval = libvirt_constcharPtrWrap(result);
    VIR_FREE(result);
    return py_retval;
}
#endif /* LIBVIR_CHECK_VERSION(0, 10, 0) */


#if LIBVIR_CHECK_VERSION(1, 2, 3)
static void
libvirt_qemu_virConnectDomainQemuMonitorEventFreeFunc(void *opaque)
{
    PyObject *pyobj_conn = (PyObject*)opaque;
    LIBVIRT_ENSURE_THREAD_STATE;
    Py_DECREF(pyobj_conn);
    LIBVIRT_RELEASE_THREAD_STATE;
}

static void
libvirt_qemu_virConnectDomainQemuMonitorEventCallback(virConnectPtr conn ATTRIBUTE_UNUSED,
                                                      virDomainPtr dom,
                                                      const char *event,
                                                      long long seconds,
                                                      unsigned int micros,
                                                      const char *details,
                                                      void *opaque)
{
    PyObject *pyobj_cbData = (PyObject*)opaque;
    PyObject *pyobj_dom;
    PyObject *pyobj_ret = NULL;
    PyObject *pyobj_conn;
    PyObject *dictKey;
    PyObject *pyobj_cb;

    LIBVIRT_ENSURE_THREAD_STATE;

    pyobj_cb = libvirt_qemu_lookupPythonFunc("_dispatchQemuMonitorEventCallback");
    if (!pyobj_cb)
        goto cleanup;

    dictKey = libvirt_constcharPtrWrap("conn");
    if (!dictKey)
        goto cleanup;
    pyobj_conn = PyDict_GetItem(pyobj_cbData, dictKey);
    Py_DECREF(dictKey);

    /* Create a python instance of this virDomainPtr */
    virDomainRef(dom);
    if (!(pyobj_dom = libvirt_virDomainPtrWrap(dom))) {
        virDomainFree(dom);
        goto cleanup;
    }
    Py_INCREF(pyobj_cbData);

    /* Call the Callback Dispatcher */
    pyobj_ret = PyObject_CallFunction(pyobj_cb,
                                      (char *)"OOsLIsO",
                                      pyobj_conn, pyobj_dom, event, seconds,
                                      micros, details, pyobj_cbData);

    Py_DECREF(pyobj_cbData);
    Py_DECREF(pyobj_dom);

 cleanup:
    if (!pyobj_ret) {
        DEBUG("%s - ret:%p\n", __FUNCTION__, pyobj_ret);
        PyErr_Print();
    } else {
        Py_DECREF(pyobj_ret);
    }

    LIBVIRT_RELEASE_THREAD_STATE;
}


static PyObject *
libvirt_qemu_virConnectDomainQemuMonitorEventRegister(PyObject *self ATTRIBUTE_UNUSED,
                                                      PyObject *args)
{
    PyObject *py_retval;
    PyObject *pyobj_conn;
    PyObject *pyobj_dom;
    PyObject *pyobj_cbData;
    const char *event;
    virConnectPtr conn;
    int ret = 0;
    virConnectDomainQemuMonitorEventCallback cb = NULL;
    virDomainPtr dom;
    unsigned int flags;

    if (!PyArg_ParseTuple(args, (char *) "OOzOI:virConnectDomainQemuMonitorEventRegister",
                          &pyobj_conn, &pyobj_dom,
                          &event, &pyobj_cbData, &flags))
        return NULL;

    DEBUG("libvirt_qemu_virConnectDomainQemuMonitorEventRegister(%p %p %s %p %x) called\n",
          pyobj_conn, pyobj_dom, NULLSTR(event), pyobj_cbData, flags);
    conn = PyvirConnect_Get(pyobj_conn);
    if (pyobj_dom == Py_None)
        dom = NULL;
    else
        dom = PyvirDomain_Get(pyobj_dom);

    cb = libvirt_qemu_virConnectDomainQemuMonitorEventCallback;

    Py_INCREF(pyobj_cbData);

    LIBVIRT_BEGIN_ALLOW_THREADS;
    ret = virConnectDomainQemuMonitorEventRegister(conn, dom, event,
                                                   cb, pyobj_cbData,
                                                   libvirt_qemu_virConnectDomainQemuMonitorEventFreeFunc,
                                                   flags);
    LIBVIRT_END_ALLOW_THREADS;

    if (ret < 0) {
        Py_DECREF(pyobj_cbData);
    }

    py_retval = libvirt_intWrap(ret);
    return py_retval;
}


static PyObject *
libvirt_qemu_virConnectDomainQemuMonitorEventDeregister(PyObject *self ATTRIBUTE_UNUSED,
                                                        PyObject *args)
{
    PyObject *py_retval;
    PyObject *pyobj_conn;
    int callbackID;
    virConnectPtr conn;
    int ret = 0;

    if (!PyArg_ParseTuple(args,
                          (char *) "Oi:virConnectDomainQemuMonitorEventDeregister",
                          &pyobj_conn, &callbackID))
        return NULL;

    DEBUG("libvirt_qemu_virConnectDomainQemuMonitorEventDeregister(%p) called\n",
          pyobj_conn);

    conn = PyvirConnect_Get(pyobj_conn);

    LIBVIRT_BEGIN_ALLOW_THREADS;

    ret = virConnectDomainQemuMonitorEventDeregister(conn, callbackID);

    LIBVIRT_END_ALLOW_THREADS;
    py_retval = libvirt_intWrap(ret);
    return py_retval;
}
#endif /* LIBVIR_CHECK_VERSION(1, 2, 3) */

#if LIBVIR_CHECK_VERSION(8, 2, 0)
static PyObject *
libvirt_qemu_virDomainQemuMonitorCommandWithFiles(PyObject *self ATTRIBUTE_UNUSED,
                                                  PyObject *args)
{
    PyObject *pyobj_domain;
    const char *cmd;
    PyObject *pyobj_files;
    unsigned int flags;
    virDomainPtr domain;
    unsigned int ninfiles;
    int *infiles = NULL;
    unsigned int noutfiles = 0;
    int *outfiles = NULL;
    char *result = NULL;
    size_t i;
    PyObject *py_outfiles = NULL;
    PyObject *py_retval = NULL;
    int c_retval;

    if (!PyArg_ParseTuple(args,
                          (char *) "Os|OI:virDomainQemuMonitorCommandWithFiles",
                          &pyobj_domain, &cmd, &pyobj_files, &flags))
        return NULL;
    domain = (virDomainPtr) PyvirDomain_Get(pyobj_domain);

    ninfiles = PyList_Size(pyobj_files);

    if (VIR_ALLOC_N(infiles, ninfiles) < 0)
        return PyErr_NoMemory();

    for (i = 0; i < ninfiles; i++) {
        PyObject *pyfd;
        int fd;

        pyfd = PyList_GetItem(pyobj_files, i);

        if (libvirt_intUnwrap(pyfd, &fd) < 0)
            goto cleanup;

        infiles[i] = fd;
    }

    LIBVIRT_BEGIN_ALLOW_THREADS;
    c_retval = virDomainQemuMonitorCommandWithFiles(domain, cmd, ninfiles, infiles,
                                                    &noutfiles, &outfiles, &result, flags);
    LIBVIRT_END_ALLOW_THREADS;

    if (c_retval < 0) {
        py_retval = VIR_PY_NONE;
        goto cleanup;
    }

    if (!(py_outfiles = PyList_New(0)) ||
        !(py_retval = PyTuple_New(2))) {
        goto error;
    }

    for (i = 0; i < noutfiles; i++) {
        int fd = outfiles[i];
        const char *mode = "r+b";

        /* Since FD passing works only on UNIX-like systems, we can do this. */
#ifndef __CYGWIN__
        int fflags;

        if ((fflags = fcntl(fd, F_GETFL)) < 0)
            goto error;

        switch (fflags & (O_ACCMODE | O_APPEND)) {
        case O_RDONLY:
            mode = "rb";
            break;
        case O_WRONLY:
            mode = "wb";
            break;
        case O_RDWR:
            mode = "r+b";
            break;
        case O_WRONLY | O_APPEND:
            mode = "ab";
            break;
        case O_RDWR | O_APPEND:
            mode = "a+b";
            break;
        }
#endif

        VIR_PY_LIST_APPEND_GOTO(py_outfiles, PyFile_FromFd(fd, NULL, mode, 0, NULL, NULL, NULL, 1), error);
    }

    VIR_PY_TUPLE_SET_GOTO(py_retval, 0, libvirt_charPtrWrap(result), error);
    VIR_PY_TUPLE_SET_GOTO(py_retval, 1, py_outfiles, error);
    /* stolen by py_retval */
    py_outfiles = NULL;

 cleanup:
    Py_XDECREF(py_outfiles);
    VIR_FREE(result);
    VIR_FREE(outfiles);
    VIR_FREE(infiles);
    return py_retval;

 error:
    while (noutfiles > 0) {
        VIR_FORCE_CLOSE(outfiles[--noutfiles]);
    }
    Py_CLEAR(py_retval);
    goto cleanup;
}
#endif /* LIBVIR_CHECK_VERSION(8, 2, 0) */

/************************************************************************
 *									*
 *			The registration stuff				*
 *									*
 ************************************************************************/
static PyMethodDef libvirtQemuMethods[] = {
#include "libvirt-qemu-export.c.inc"
    {(char *) "virDomainQemuMonitorCommand", libvirt_qemu_virDomainQemuMonitorCommand, METH_VARARGS, NULL},
#if LIBVIR_CHECK_VERSION(0, 10, 0)
    {(char *) "virDomainQemuAgentCommand", libvirt_qemu_virDomainQemuAgentCommand, METH_VARARGS, NULL},
#endif /* LIBVIR_CHECK_VERSION(0, 10, 0) */
#if LIBVIR_CHECK_VERSION(1, 2, 3)
    {(char *) "virConnectDomainQemuMonitorEventRegister", libvirt_qemu_virConnectDomainQemuMonitorEventRegister, METH_VARARGS, NULL},
    {(char *) "virConnectDomainQemuMonitorEventDeregister", libvirt_qemu_virConnectDomainQemuMonitorEventDeregister, METH_VARARGS, NULL},
#endif /* LIBVIR_CHECK_VERSION(1, 2, 3) */
#if LIBVIR_CHECK_VERSION(8, 2, 0)
    {(char *) "virDomainQemuMonitorCommandWithFiles", libvirt_qemu_virDomainQemuMonitorCommandWithFiles, METH_VARARGS, NULL},
#endif /* LIBVIR_CHECK_VERSION(8, 2, 0) */
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
#ifndef __CYGWIN__
    "libvirtmod_qemu",
#else
    "cygvirtmod_qemu",
#endif
    NULL,
    -1,
    libvirtQemuMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
#ifndef __CYGWIN__
PyInit_libvirtmod_qemu
#else
PyInit_cygvirtmod_qemu
#endif
(void)
{
    PyObject *module;

    if (virInitialize() < 0)
        return NULL;

    module = PyModule_Create(&moduledef);

    return module;
}
