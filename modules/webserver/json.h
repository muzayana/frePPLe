/***************************************************************************
 *                                                                         *
 * Copyright (C) 2014-2015 by frePPLe bvba                                 *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

/** @file json.h
  * @brief Header file for handling data in JSON format.
  *
  * It implements a fast JSON serializer, and a SAX-style JSON deserializer.
  */

#ifndef JSON_H
#define JSON_H

#include "frepple.h"
using namespace frepple;

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef __int64   int64_t;
#endif

#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"

namespace module_webserver
{


/** @brief Base class for writing JSON formatted data to an output stream.
  *
  * Subclasses implement writing to specific stream types, such as files
  * and strings.
  */
class JSONSerializer : public Serializer
{
  public:
    /** Constructor with a given stream. */
    JSONSerializer(ostream& os) : Serializer(os), formatted(false), first(true), m_nIndent(0)
    {
      indentstring[0] = '\0';
    }

    /** Default constructor. */
    JSONSerializer() : formatted(false), first(true), m_nIndent(0)
    {
      indentstring[0] = '\0';
    }

    /** Tweak to toggle between the dictionary and array modes. */
    void setMode(bool f)
    {
      if (mode.empty())
        mode.push(f);
      else
        mode.top() = f;
    }

    void setFormatted(bool b)
    {
      formatted = b;
    }

    bool getFormatted() const
    {
      return formatted;
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {
      */
    void BeginList(const Keyword& t)
    {
      if (formatted)
      {
        if (!first)
          *m_fp << ",\n" << indentstring;
        incIndent();
      }
      else if (!first)
        *m_fp << ",";
      *m_fp << t.getQuoted() << "[";
      first = true;
      mode.push(true);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {
      */
    void BeginObject(const Keyword& t)
    {
      if (formatted)
      {
        if (!first)
          *m_fp << ",\n" << indentstring;
        incIndent();
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.empty() && !mode.top())
        *m_fp << t.getQuoted();
      if (formatted)
        *m_fp << "{\n" << indentstring;
      else
        *m_fp << "{";
      first = true;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.
      * Output: "TAG" : {
      */
    void BeginObject(const Keyword& t, const string& atts)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      *m_fp << t.getQuoted() << "{";
      first = true;
      mode.push(false);
      logger << "IMPLEMENTATION INCOMPLETE" << endl; // TODO not using atts
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {"TAG1": VAL1    (dictionary mode)
      *         {"TAG1": VAL1            (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const string& val1)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{" << attr1.getQuoted();
      escape(val1);
      first = false;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {"TAG1": VAL1    (dictionary mode)
      *         {"TAG1": VAL1            (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const int val1)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{" << attr1.getQuoted() << val1;
      first = false;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {"TAG1": VAL1    (dictionary mode)
      *         {"TAG1": VAL1            (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const Date val1)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{" << attr1.getQuoted() << val1;
      first = false;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG":{"TAG1":"VAL1","TAG2":"VAL2" (dictionary mode)
      *         {"TAG1":"VAL1","TAG2":"VAL2"         (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const string& val1,
      const Keyword& attr2, const string& val2)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{" << attr1.getQuoted();
      escape(val1);
      *m_fp << "," << attr2.getQuoted();
      escape(val2);
      first = false;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG":{"TAG1":"VAL1","TAG2":"VAL2" (dictionary mode)
      *         {"TAG1":"VAL1","TAG2":"VAL2"         (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const unsigned long& val1,
      const Keyword& attr2, const string& val2)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{" << attr1.getQuoted()
        << val1 << "," << attr2.getQuoted();
      escape(val2);
      first = false;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG":{"TAG1":"VAL1","TAG2":"VAL2" (dictionary mode)
      *         {"TAG1":"VAL1","TAG2":"VAL2"         (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const int& val1,
      const Keyword& attr2, const Date val2,
      const Keyword& attr3, const Date val3)
    {
      if (formatted)
      {
        incIndent();
        if (first)
          *m_fp << "\n" << indentstring;
        else
          *m_fp << ",\n" << indentstring;
      }
      else if (!first)
        *m_fp << ",";
      if (!mode.top())
        *m_fp << t.getQuoted();
      *m_fp << "{"
        << attr1.getQuoted() << val1 << ","
        << attr2.getQuoted() << "\"" << val2 << "\","
        << attr3.getQuoted() << "\"" << val3 << "\"";
      first = false;
      mode.push(false);
    }

    /** Write the closing tag of this object<br>
      * Output: }
      */
    void EndObject(const Keyword& t)
    {
      if (formatted)
      {
        decIndent();
        *m_fp << "\n" << indentstring << "}";
      }
      else
        *m_fp << "}";
      first = false;
      mode.pop();
    }

    /** Write the closing tag of this object<br>
      * Output: }
      */
    void EndList(const Keyword& t)
    {
      if (formatted)
        decIndent();
      *m_fp << "]";
      first = false;
      mode.pop();
    }

    /** Write the string to the output. This method is used for passing text
      * straight into the output file.
      */
    void writeString(const string& c)
    {
      *m_fp << c;
    }

    /** Write an unsigned long value enclosed opening and closing tags.<br>
      * Output: , "TAG": uint
      */
    void writeElement(const Keyword& t, const long unsigned int val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << val;
    }

    /** Write an integer value enclosed opening and closing tags.<br>
      * Output: ,"TAG": int
      */
    void writeElement(const Keyword& t, const int val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << val;
    }

    /** Write a double value enclosed opening and closing tags.<br>
      * Output: ,"TAG": double
      */
    void writeElement(const Keyword& t, const double val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << val;
    }

    /** Write a boolean value enclosed opening and closing tags. The boolean
      * is written out as the string 'true' or 'false'.<br>
      * Output: "TAG": true/false
      */
    void writeElement(const Keyword& t, const bool val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << (val ? "true" : "false");
    }

    /** Write a string value enclosed opening and closing tags.<br>
      * Output: "TAG": "val"
      */
    void writeElement(const Keyword& t, const string& val)
    {
      if (val.empty()) return;
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted();
      escape(val);
    }

    /** Writes an element with a string attribute.<br>
      * Output:
      *   "TAG_U": {"TAG_T": "string"}    (dictionary mode)
      *   {"TAG_T": "string"}             (array mode)
      */
    void writeElement(const Keyword& u, const Keyword& t, const string& val)
    {
      if (val.empty())
      {
        if (!mode.top())
        {
          if (first)
            first = false;
          else if (formatted)
            *m_fp << ",\n" << indentstring;
          else
            *m_fp << ",";
          *m_fp << u.getQuoted() << "{}";
        }
      }
      else
      {
        if (first)
          first = false;
        else if (formatted)
          *m_fp << ",\n" << indentstring;
        else
          *m_fp << ",";
        if (!mode.top())
          *m_fp << u.getQuoted();
        *m_fp << "{" << t.getQuoted();
        escape(val);
        *m_fp << "}";
      }
    }

    /** Writes an element with a long attribute.<br>
      * Output: "TAG_U": {"TAG_T": long}
      */
    void writeElement(const Keyword& u, const Keyword& t, const long val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      if (!mode.top())
        *m_fp << u.getQuoted();
      *m_fp << "{" << t.getQuoted() << val << "}";
    }

    /** Writes an element with a date attribute.<br>
      * Output: "TAG_U": {"TAG_T": date}
      */
    void writeElement(const Keyword& u, const Keyword& t, const Date& val)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      if (!mode.top())
        *m_fp << u.getQuoted();
      *m_fp << "{" << t.getQuoted() << val << "}";
    }

    /** Writes an element with 2 string attributes.<br>
      * Output: "TAG_U":{"TAG_T1":"val1","TAGT2":"val2"}
      */
    void writeElement(const Keyword& u, const Keyword& t1, const string& val1,
        const Keyword& t2, const string& val2)
    {
      if (val1.empty())
      {
        if (!mode.top())
        {
          if (first)
            first = false;
          else if (formatted)
            *m_fp << ",\n" << indentstring;
          else
            *m_fp << ",";
          *m_fp << u.getQuoted() << "{}";
        }
      }
      else
      {
        if (first)
          first = false;
        else if (formatted)
          *m_fp << ",\n" << indentstring;
        else
          *m_fp << ",";
        if (!mode.top())
          *m_fp << u.getQuoted();
        *m_fp << "{" << t1.getQuoted();
        escape(val1);
        *m_fp << "," << t2.getQuoted();
        escape(val2);
        *m_fp << "}";
      }
    }

    /** Writes an element with a string and an unsigned long attribute.<br>
      * Output: "TAG_U": {"TAG_T1": "val1","TAGT2": "val2"}
      */
    void writeElement(const Keyword& u, const Keyword& t1, unsigned long val1,
        const Keyword& t2, const string& val2)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      if (!mode.top())
        *m_fp << u.getQuoted();
      *m_fp << "{" << t1.getQuoted() << val1
        << "," << t2.getQuoted();
      escape(val2);
      *m_fp << "}";
    }

    /** Writes an element with a short, an unsigned long and a double attribute.<br>
      * Output: "TAG_U": {"TAG_T1":val1,"TAGT2":val2,"TAGT3":val3}
      */
    void writeElement(const Keyword& u, const Keyword& t1, short val1,
        const Keyword& t2, unsigned long val2, const Keyword& t3, double val3)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      if (!mode.top())
        *m_fp << u.getQuoted();
      *m_fp << "{" << t1.getQuoted() << val1
        << "," << t2.getQuoted() << val2
        << "," << t3.getQuoted() << val3
        << "}";
    }

    /** Writes a C-type character string.<br>
      * Output: "TAG_T": "val"
      */
    void writeElement(const Keyword& t, const char* val)
    {
      if (!val) return;
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted();
      escape(val);
    }

    /** Writes an timeperiod element.<br>
      * Output: "TAG_T": "val"
      */
    void writeElement(const Keyword& t, const Duration d)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << "\"" << d << "\"";
    }

    /** Writes an date element.<br>
      * Output: \<TAG_T\>d\</TAG_T\> /> */
    void writeElement(const Keyword& t, const Date d)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << "\"" << d << "\"";
    }

    /** Writes an daterange element.<br>
      * Output: \<TAG_T\>d\</TAG_T\> */
    void writeElement(const Keyword& t, const DateRange& d)
    {
      if (first)
        first = false;
      else if (formatted)
        *m_fp << ",\n" << indentstring;
      else
        *m_fp << ",";
      *m_fp << t.getQuoted() << "\"" << d << "\"";
    }

  private:
    /** Write the argument to the output stream, while escaping any
      * special characters.
      * From the JSON specification http://www.ietf.org/rfc/rfc4627.txt:
      *   All Unicode characters may be placed within the quotation marks
      *   except for the characters that must be escaped: quotation mark,
      *   reverse solidus, and the control characters (U+0000 through
      *   U+001F).
      * For convenience we also escape the forward slash.
      *
      * This method works fine with UTF-8 and single-byte encodings, but will
      * NOT work with other multibyte encodings (such as UTF-116 or UTF-32).
      * FrePPLe consistently uses UTF-8 in its internal representation.
      */
    void escape(const string&);

    /** Generated nicely formatted text.
      * This is false by default, because it generates a smaller file
      * without the extra whitespace.
      */
    bool formatted;

    /** Flag to mark if an object has already one or more fields saved. */
    bool first;

    /** Stack to keep track of the current output mode: dictionary (true)
      * or array (true).
      */
    stack<bool> mode;

    /** This string is a null terminated string containing as many spaces as
      * indicated by the m_nIndent.
      * @see incIndent, decIndent
      */
    char indentstring[41];

    /** This variable keeps track of the indentation level.
      * @see incIndent, decIndent
      */
    short int m_nIndent;

    /** Increment indentation level in the formatted output. */
    inline void incIndent()
    {
      indentstring[m_nIndent++] = '\t';
      if (m_nIndent > 40) m_nIndent = 40;
      indentstring[m_nIndent] = '\0';
    }

    /** Decrement indentation level in the formatted output. */
    inline void decIndent()
    {
      if (--m_nIndent < 0) m_nIndent = 0;
      indentstring[m_nIndent] = '\0';
    }

    /** Stack of objects and their data fields. */
    struct obj
    {
      const MetaClass* cls;
      Object* object;
      int start;
      hashtype hash;
    };
    vector<obj> objects;
};


/** @brief This class writes JSON data to a flat file.
  *
  * Note that an object of this class can write only to a single file. If
  * you need to write multiple files then multiple JSONSerializerFile objects
  * will be required.
  */
class JSONSerializerFile : public JSONSerializer
{
  public:
    /** Constructor with a filename as argument. An exception will be
      * thrown if the output file can't be properly initialized. */
    JSONSerializerFile(const string& chFilename)
    {
      of.open(chFilename.c_str(), ios::out);
      if(!of)
        throw RuntimeException("Could not open output file");
      setOutput(of);
    }

    /** Destructor. */
    ~JSONSerializerFile()
    {
      of.close();
    }

  private:
    ofstream of;
};


/** @brief This class writes JSON data to a string.
  *
  * The generated output is stored internally in the class, and can be
  * accessed by converting the XMLOutputString object to a string object.
  * This class can consume a lot of memory if large sets of objects are
  * being saved in this way.
  */
class JSONSerializerString : public JSONSerializer
{
  public:
    /** Default constructor. */
    JSONSerializerString()
    {
      setOutput(os);
    }

    /** Return the output string. */
    const string getData() const
    {
      return os.str();
    }

  private:
    ostringstream os;
};


/** @brief This python function writes the complete model to a JSON-file.
  *
  * Both the static model (i.e. items, locations, buffers, resources,
  * calendars, etc...) and the dynamic data (i.e. the actual plan including
  * the operationplans, demand, problems, etc...).<br>
  * The format is such that the output file can be re-read to restore the
  * very same model.<br>
  * The function takes the following arguments:
  *   - Name of the output file
  *   - Type of output desired: BASE, PLAN or DETAIL.
  *     The default value is BASE.
  */
PyObject* saveJSONfile(PyObject* self, PyObject* args);


class JSONData : public DataValue
{
  public:
    /** Field types recognized by the parser. */
    enum JsonType
    {
      JSON_NULL,
      JSON_BOOL,
      JSON_INT,
      JSON_LONG,
      JSON_UNSIGNEDLONG,
      JSON_DOUBLE,
      JSON_STRING,
      JSON_OBJECT
    };

    /** Constructor. */
    JSONData() : data_type(JSON_NULL) {}

    /** Destructor. */
    virtual ~JSONData() {}

    virtual operator bool() const
    {
      return getBool();
    }

    virtual long getLong() const;

    virtual unsigned long getUnsignedLong() const;

    virtual Duration getDuration() const;

    virtual int getInt() const;

    virtual double getDouble() const;

    virtual Date getDate() const;

    virtual const string& getString() const;

    virtual bool getBool() const;

    virtual Object* getObject() const;

    void setNull()
    {
      data_type = JSON_NULL;
    }

    virtual void setLong(const long l)
    {
      data_type = JSON_LONG;
      data_long = l;
    }

    virtual void setUnsignedLong(const unsigned long ul)
    {
      data_type = JSON_UNSIGNEDLONG;
      data_long = ul;
    }

    virtual void setDuration(const Duration d)
    {
      data_type = JSON_LONG;
      data_long = static_cast<long>(d);
    }

    virtual void setInt(const int i)
    {
      data_type = JSON_INT;
      data_int = i;
    }

    virtual void setDouble(const double d)
    {
      data_type = JSON_DOUBLE;
      data_double = d;
    }

    virtual void setDate(const Date d)
    {
      data_type = JSON_LONG;
      data_long = d.getTicks();
    }

    virtual void setString(const string& s)
    {
      data_type = JSON_STRING;
      data_string = s;
    }

    virtual void setBool(const bool b)
    {
      data_type = JSON_BOOL;
      data_bool = b;
    }

    virtual void setObject(Object* o)
    {
      data_type = JSON_OBJECT;
      data_object = o;
    }

    JsonType getDataType() const
    {
      return data_type;
    }

  private:
    /** Stores the type of data we're storing. */
    JsonType data_type;

    /** Data content. */
    union
    {
      bool data_bool;
      int data_int;
      long data_long;
      unsigned long data_unsignedlong;
      double data_double;
      Object* data_object;
    };
    string data_string;
};


/** @brief A JSON parser, using the rapidjson library.
  *
  * Some specific limitations of the implementation:
  *   - JSON allows NULLs in the string values.
  *     FrePPLe doesn't, and we will only consider the part before the
  *     null characters.
  *   - The parser only supports UTF-8 encodings.
  *     RapidJSON also supports UTF-16 and UTF-32 (LE & BE), but a) FrePPLe
  *     internally represents all string data as UTF-8 and b) the in-situ
  *     parser requires input and destination encoding to be the same.
  *
  * See https://github.com/miloyip/rapidjson for information on rapidjson,
  * which is released under the MIT license.
  */
class JSONInput : public NonCopyable
{
  friend rapidjson::Reader;
  private:
    /** This variable defines the maximum depth of the object creation stack.
      * This maximum is intended to protect us from malicious malformed
      * documents, and also for allocating efficient data structures for
      * the parser.
      */
    static const int maxobjects = 30;
    static const int maxdata = 200;

  public:
    struct fld
    {
      const MetaFieldBase* field;
      hashtype hash;
      JSONData value;
      string name;
    };

  private:
    /** Stack of fields already read. */
    vector<fld> data;

    /** Stack of objects and their data fields. */
    struct obj
    {
      const MetaClass* cls;
      Object* object;
      int start;
      hashtype hash;
    };
    vector<obj> objects;

    /** Index into the objects stack. */
    int objectindex;

    /** Index into the data field stack. */
    int dataindex;

    // Handler callback functions for rapidjson
    bool Null();
    bool Bool(bool b);
    bool Int(int i);
    bool Uint(unsigned u);
    bool Int64(int64_t i);
    bool Uint64(uint64_t u);
    bool Double(double d);
    bool String(const char* str, rapidjson::SizeType length, bool copy);
    bool StartObject();
    bool Key(const char* str, rapidjson::SizeType length, bool copy);
    bool EndObject(rapidjson::SizeType memberCount);
    bool StartArray();
    bool EndArray(rapidjson::SizeType elementCount);

  protected:
    /** Constructor. */
    JSONInput() : data(maxdata), objects(maxobjects) {}

    /** Main parser function. */
    void parse(Object* pRoot, char* buffer);
};


/** @brief This class reads JSON data from a string. */
class JSONInputString : public JSONInput
{
  public:
    /** Default constructor. */
    JSONInputString(char* s) : data(s) {};

    /** Parse the specified string. */
    void parse(Object* pRoot)
    {
      JSONInput::parse(pRoot, data);
    }

  private:
    /** String containing the data to be parsed. Note that NO local copy of the
      * data is made, only a reference is stored. The class relies on the code
      * calling the command to correctly create and destroy the string being
      * used.
      */
    char* data;
};


/** @brief This class reads JSON data from a file system.
  *
  * The filename argument can be the name of a file or a directory.
  * If a directory is passed, all files with the extension ".json"
  * will be read from it. Subdirectories are not recursed.
  */
class JSONInputFile : public JSONInput
{
  public:
    /** Constructor. The argument passed is the name of a
      * file or a directory. */
    JSONInputFile(const string& s) : filename(s) {};

    /** Default constructor. */
    JSONInputFile() {};

    /** Update the name of the file to be processed. */
    void setFileName(const string& s)
    {
      filename = s;
    }

    /** Returns the name of the file or directory to process. */
    string getFileName()
    {
      return filename;
    }

    /** Parse the specified file.
      * When a directory was passed as the argument a failure is
      * flagged as soon as a single file returned a failure. All
      * files in an directory are processed however, regardless of
      * failure with one of the files.
      * @exception RuntimeException Generated in the following conditions:
      *    - no input file or directory has been specified.
      *    - read access to the input file is not available
      *    - the program doesn't support reading directories on your platform
      */
    void parse(Object*);

  private:
    /** Name of the file or directory to be opened. */
    string filename;
};



/** @brief This class represents a list of JSON key+value pairs.
  *
  * The method is a thin wrapper around one of the internal data
  * structures of the parser implemented in the class JSONInput.
  */
class JSONDataValueDict : public DataValueDict
{
  public:
    typedef vector< pair<DataKeyword, XMLData> > dict;

    /** Constructor. */
    JSONDataValueDict(
      vector<JSONInput::fld>& f,
      int st,
      int nd
      ) : fields(f), strt(st), nd(nd)
    {
      if (strt < 0)
        strt = 0;
    }

    /** Look up a certain keyword. */
    const JSONData* get(const Keyword& key) const;

    /** Enlarge the dictiorary. */
    void enlarge()
    {
      ++nd;
    }

    /** Auxilary debugging method. */
    void print();

    /** Return the start index in the array. */
    inline int getStart() const
    {
      return strt;
    }

    /** Return the end index in the array. */
    inline int getEnd() const
    {
      return nd;
    }
  private:
    vector<JSONInput::fld>& fields;
    int strt;
    int nd;
};


/** Method exposed in Python to process JSON data from a string. */
PyObject* readJSONdata(PyObject*, PyObject*);


/** Method exposed in Python to process JSON data from a file. */
PyObject* readJSONfile(PyObject*, PyObject*);


}   // End namespace

#endif


