/***************************************************************************
 *                                                                         *
 * Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/


#ifndef SAMPLE_MODULE_H
#define SAMPLE_MODULE_H
#include "frepple.h"
using namespace frepple;

/** @file sample_module.h
  * @brief Header file for a sample module.
  *
  * @namespace sample_module
  * @brief A sample module.
  *
  * Using a seperate namespace keeps things clean and simple.
  * It keeps the code structured. The auto-generated documentation will also
  * follow the same modular structure as your extension modules.
  */
namespace sample_module
{


/** This is the initialization routine for the extension.
  * Including a function with this prototype is compulsary. If it doesn't
  * exist your module will not be able to be loaded.
  * The function is called automatically when your module is loaded.
  *
  * Parameters can be passed when loading the library.
  *
  * The initialization routine returns a pointer to a constant character
  * buffer with the module name.
  */
MODULE_EXPORT const char* initialize(const Environment::ParameterList& z);


/** @brief Operation type for modeling transportation efficiently.
  *
  * A transport operation has the following characteristics:
  *  - consumes 1 unit from a source buffer
  *  - produces 1 unit into a destination buffer
  *  - takes a fixed duration of time
  */
class OperationTransport : public OperationFixedTime
{
  private:
    /** Buffer at the source location. */
    Buffer* fromBuf;

    /** Buffer at the destination location. */
    Buffer* toBuf;

    static const Keyword tag_frombuffer;
    static const Keyword tag_tobuffer;

  public:
    /** Constructor. */
    explicit OperationTransport() : fromBuf(NULL), toBuf(NULL)
    {
      initType(metadata);
    }

    /** Returns a pointer to the source buffer. */
    Buffer* getFromBuffer() const
    {
      return fromBuf;
    }

    /** Update the source buffer of the transport.<br>
      * If operationplans already exist for the operation the update will
      * fail.
      */
    void setFromBuffer(Buffer *l);

    /** Returns a pointer to the destination buffer. */
    Buffer* getToBuffer() const
    {
      return toBuf;
    }

    /** Update the destination buffer of the transport.<br>
      * If operationplans already exist for the operation the update will
      * fail.
      */
    void setToBuffer(Buffer *l);

    /** Returns a reference to this class' metadata. */
    virtual const MetaClass& getType() const {return *metadata;}

    /** A static metadata object. */
    static const MetaClass *metadata;

    /** This callback will automatically be called when a buffer is deleted. */
    static bool callback(Buffer*, const Signal);

    template<class Cls> static inline void registerFields(MetaClass* m)
    {
      OperationFixedTime::registerFields<OperationTransport>(m);
      m->addPointerField<Cls, Buffer>(tag_frombuffer, &Cls::getFromBuffer, &Cls::setFromBuffer);
      m->addPointerField<Cls, Buffer>(tag_tobuffer, &Cls::getToBuffer, &Cls::setToBuffer);
    }
};

} // End namespace

#endif   // endif SAMPLE_MODULE_H
