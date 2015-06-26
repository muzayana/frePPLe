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

#if defined(HAVE_STDINT)
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef __int64   int64_t;
#endif

#include "frepple.h"
using namespace frepple;

namespace module_webserver
{


/** @brief Base class for writing JSON formatted data to an output stream.
  *
  * Subclasses implement writing to specific stream types, such as files
  * and strings.
  */
class SerializerJSON : public Serializer
{
  public:
    /** Constructor with a given stream. */
    SerializerJSON(ostream& os) : Serializer(os), first(true) {}

    /** Default constructor. */
    SerializerJSON() : first(true) {}

    /** Tweak to toggle between the dictionary and array modes. */
    void setMode(bool f)
    {
      if (mode.empty())
        mode.push(f);
      else
        mode.top() = f;
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {
      */
    void BeginList(const Keyword& t)
    {
      if (!first)
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
      if (!first)
        *m_fp << ",";
      if (mode.empty() || mode.top())
        *m_fp << "{";
      else
        *m_fp << t.getQuoted() << "{";
      first = true;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.
      * Output: "TAG" : {
      */
    void BeginObject(const Keyword& t, const string& atts)
    {
      if (!first)
        *m_fp << ",";
      *m_fp << t.getQuoted() << "{";
      first = true;
      mode.push(false);
    }

    /** Start writing a new object. This method will open a new tag.<br>
      * Output: "TAG" : {"TAG1": VAL1    (dictionary mode)
      *         {"TAG1": VAL1            (array mode)
      */
    void BeginObject(const Keyword& t, const Keyword& attr1, const string& val1)
    {
      if (!first)
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
      if (!first)
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
      if (!first)
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
      if (!first)
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
      if (!first)
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
      if (!first)
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
      *m_fp << "}";
      first = false;
      mode.pop();
    }

    /** Write the closing tag of this object<br>
      * Output: }
      */
    void EndList(const Keyword& t)
    {
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
          else
            *m_fp << ",";
          *m_fp << u.getQuoted() << "{}";
        }
      }
      else
      {
        if (first)
          first = false;
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
          else
            *m_fp << ",";
          *m_fp << u.getQuoted() << "{}";
        }
      }
      else
      {
        if (first)
          first = false;
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

    /** Flag to mark if an object has already one or more fields saved. */
    bool first;

    /** Stack to keep track of the current output mode: dictionary (true)
      * or array (true).
      */
    stack<bool> mode;
};


/** @brief This class writes JSON data to a flat file.
  *
  * Note that an object of this class can write only to a single file. If
  * multiple files are required multiple SerializerJSONFile objects will be
  * required too.
  */
class SerializerJSONFile : public SerializerJSON
{
  public:
    /** Constructor with a filename as argument. An exception will be
      * thrown if the output file can't be properly initialized. */
    SerializerJSONFile(const string& chFilename)
    {
      of.open(chFilename.c_str(), ios::out);
      if(!of) throw RuntimeException("Could not open output file");
      setOutput(of);
    }

    /** Destructor. */
    ~SerializerJSONFile() {of.close();}

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
class SerializerJSONString : public SerializerJSON
{
  public:
    /** Default constructor. */
    SerializerJSONString() {setOutput(os);}

    /** Return the output string. */
    const string getData() const {return os.str();}

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
  *   - Type of output desired: STANDARD, PLAN or PLANDETAIL.
  *     The default value is STANDARD.
  */
PyObject* saveJSONfile(PyObject* self, PyObject* args);


/** @brief A fast JSON parser.
  *
  * The parser only supports UTF-8 data.
  *
  * The code is inspired on gason https://github.com/vivkin/gason, which
  * is released under the MIT license.
  */
class JSONInput : public NonCopyable
{
  public:
    struct JsonNode;
  private:
    enum JsonTag
    {
      JSON_NUMBER = 0,
      JSON_STRING,
      JSON_ARRAY,
      JSON_OBJECT,
      JSON_TRUE,
      JSON_FALSE,
      JSON_NULL = 0xF
    };

    #define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
    #define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
    #define JSON_VALUE_TAG_MASK 0xF
    #define JSON_VALUE_TAG_SHIFT 47

    union JsonValue
    {
      uint64_t ival;
      double fval;

      JsonValue(double x) : fval(x) { }

      JsonValue(JsonTag tag = JSON_NULL, void *payload = NULL)
      {
        assert((uint64_t)payload <= JSON_VALUE_PAYLOAD_MASK);
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)tag << JSON_VALUE_TAG_SHIFT) | (uint64_t)payload;
      }

      bool isDouble() const
      {
        return (int64_t)ival <= (int64_t)JSON_VALUE_NAN_MASK;
      }

      JsonTag getTag() const
      {
        return isDouble() ? JSON_NUMBER : JsonTag((ival >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
      }

      uint64_t getPayload() const
      {
        assert(!isDouble());
        return ival & JSON_VALUE_PAYLOAD_MASK;
      }

      double toNumber() const
      {
          assert(getTag() == JSON_NUMBER);
          return fval;
      }

      char *toString() const
      {
          assert(getTag() == JSON_STRING);
          return (char*)getPayload();
      }

      JsonNode *toNode() const
      {
          assert(getTag() == JSON_ARRAY || getTag() == JSON_OBJECT);
          return (JsonNode*)getPayload();
      }
    };

  public:
    /** Node in the document. */
    struct JsonNode
    {
      JsonValue value;
      JsonNode *next;
      char *key;
    };

    /** Iterator. */
    class JsonIterator
    {
      public:
        JsonNode *p;

        JsonIterator(JsonNode* o) : p(o) {}

        void operator++() { p = p->next; }

        bool operator!=(const JsonIterator &x) const { return p != x.p; }

        JsonNode *operator*() const { return p; }

        JsonNode *operator->() const { return p; }
    };

    inline JsonIterator begin(JsonValue o) { return o.toNode(); }

    inline JsonIterator end() { return NULL; }

  private:
    class JsonAllocator
    {
      private:
        struct Zone
        {
          Zone *next;
          size_t used;
        } *head;

      public:
        JsonAllocator() : head(NULL) {}
        /*TODO
        JsonAllocator(const JsonAllocator &) = delete;
        JsonAllocator &operator=(const JsonAllocator &) = delete;
        JsonAllocator(JsonAllocator &&x) : head(x.head) {
            x.head = NULL;
        }
        JsonAllocator &operator=(JsonAllocator &&x) {
            head = x.head;
            x.head = nullptr;
            return *this;
        }
        */
        ~JsonAllocator() { deallocate(); }
        void *allocate(size_t size);
        void deallocate();
    };

    static inline bool isspace(char c)
    {
        return c == ' ' || (c >= '\t' && c <= '\r');
    }

    static inline bool isdelim(char c)
    {
        return c == ',' || c == ':' || c == ']' || c == '}' || isspace(c) || !c;
    }

    static inline bool isdigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    static inline bool isxdigit(char c)
    {
        return (c >= '0' && c <= '9') || ((c & ~' ') >= 'A' && (c & ~' ') <= 'F');
    }

    static inline int char2int(char c)
    {
        if (c <= '9')
            return c - '0';
        return (c & ~' ') - 'A' + 10;
    }

    static double string2double(char *s, char **endptr);

    static inline JsonNode *insertAfter(JsonNode *tail, JsonNode *node)
    {
      if (!tail) return node->next = node;
      node->next = tail->next;
      tail->next = node;
      return node;
    }

    static inline JsonValue listToValue(JsonTag tag, JsonNode *tail)
    {
      if (tail)
      {
          auto head = tail->next;
          tail->next = nullptr;
          return JsonValue(tag, head);
      }
      return JsonValue(tag, nullptr);
    }

    /** Parsing routine. */
    void parse(char *str, JsonValue *value, JsonAllocator &allocator);

    /** Visitor function. */
    void visit(Object*, JsonValue);

  protected:
    void parse(Object*, char*);
};


class JSONAttributeList : public AttributeList
{
  private:
    JSONInput::JsonNode *node;
  public:
    virtual const DataElement* get(const Keyword&) const;
};


class JSONElement : public DataElement
{
  public:
    virtual operator bool() const;

    /** Destructor. */
    virtual ~JSONElement() {}

    virtual long getLong() const;

    virtual unsigned long getUnsignedLong() const;

    virtual Duration getDuration() const;

    virtual int getInt() const;

    virtual double getDouble() const;

    virtual Date getDate() const;

    virtual string getString() const;

    virtual bool getBool() const;
};


/** @brief This class reads JSON data from a string. */
class JSONInputString : public JSONInput
{
  public:
    /** Default constructor. */
    JSONInputString(char* s) : data(s) {};

    /** Parse the specified string. */
    void parse(Object* pRoot) { JSONInput::parse(pRoot, data); }

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
    void setFileName(const string& s) {filename = s;}

    /** Returns the name of the file or directory to process. */
    string getFileName() {return filename;}

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


PyObject* readJSONdata(PyObject*, PyObject*);


PyObject* readJSONfile(PyObject*, PyObject*);


}   // End namespace

#endif


