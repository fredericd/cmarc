#include <list>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"


using namespace std;
using namespace rapidxml;

namespace Marc {


struct Subfield {
    char letter;
    string value;
};


struct CField {
    string tag;
    string value;
};


class Field {
public:
    string tag;
    char ind1;
    char ind2;
    list<Subfield> subfields;

    Field(string t) {
        tag = t;
        ind1 = ' ';
        ind2 = ' ';
    }

    Field(string t, char i1, char i2) {
        tag = t;
        ind1 = i1;
        ind2 = i2;
    }

    Field(string& t, list<Subfield>& s) {
        tag = t;
        ind1 = ind2 = ' ';
        subfields = s;
    }

    Field(string& t, char i1, char i2,list<Subfield>& s) {
        tag = t;
        ind1 = i1;
        ind2 = i2;
        subfields = s;
    }

};


class Record {
public:
    std::list<CField> cfields;
    std::list<Field> fields;

    void setLeader(char* raw) {
        for (int i=0; i < 24; i++)
            leader[i] = raw[i];
    }

    string getLeader() {
        return string(leader, 24);
    }

private:
    char leader[24];
};


class Iso2709Parser {
private:
    static const char SS = '\x1F'; // Subfields separatoir
    static const char FT = '\x1E'; // Fields separator
    static const char RT = '\x1D'; // Records separator


    inline int parseInt(char* raw, int len) {
        int i = 0;
        while (len--)
            i = i*10 + *raw++ - 48;
        return i;
    }

public:
    Record* parse(char* raw) {
        Record* record = new Record();
        record->setLeader(raw);
        int directoryLen = parseInt(raw+12, 5) - 25;
        int numberOfTag = directoryLen / 12; 
        for (int i = 0; i < numberOfTag; i++) {
            int off = 24 + i * 12;
            string tag = string(raw + off, 3);
            int len = parseInt(raw+off+3, 4);
            int pos = parseInt(raw+off+7, 5) + 25 + directoryLen;
            char* start = raw + pos;
            if (raw[off] == '0' && raw[off+1] == '0') {
                CField field = {tag, string(start, len-1)};
                record->cfields.push_back(field);
            } else {
                Field field = Field(tag, *start, start[1]);
                start += 4;
                while (true) {
                    char *end = start;
                    while (*end != SS && *end != FT)
                        end++;
                    Subfield sf = { start[-1], string(start, end-start) };
                    field.subfields.push_back(sf);
                    if (*end == FT) break;
                    start = end + 2;
                }
                record->fields.push_back(field);
            }
        }
        return record;
    }
};


class Iso2709Reader {
private:
    std::istream* ist;
    int count;
    char buffer[100000];
    Marc::Iso2709Parser parser;

public:
    Iso2709Reader(std::ifstream* ii) {
        ist = ii;
        count = 0;
    }

    bool operator>>( Record*& record ) {
        ist->getline(buffer, sizeof(buffer), '\x1d');
        if (ist->gcount() == 0) return false;
        record = parser.parse(buffer);
        count++;
        return true;
    }

};



class Writer {
public:
    virtual void operator<<( Record* record ) {};
};


class XmlWriter : Writer {
private:
    std::ostream* ost;
    int count;


public:
    XmlWriter(std::ostream* o) {
        ost = o;
        count = 0;
        *ost << "<collection xmlns=\"http://www.loc.gov/MARC21/slim\">\n";
    }

    ~XmlWriter() {
        *ost << "</collection>\n";
    }

    void operator<<( Record* record ) {
        xml_document<> doc;    // character type defaults to char
        xml_node<> *nodeRecord = doc.allocate_node(node_element, "record");
        doc.append_node(nodeRecord);

        xml_node<> *node = doc.allocate_node(
            node_element, "leader", doc.allocate_string(record->getLeader().c_str()));
        nodeRecord->append_node(node);

        for (list<CField>::const_iterator i = record->cfields.begin();
             i != record->cfields.end(); ++i)
        {
            CField f = *i;
            xml_node<> *node = doc.allocate_node(
                node_element, "controlfield", f.value.c_str());
            xml_attribute<> *attr = doc.allocate_attribute(
                "tag", string(f.tag).c_str());
            node->append_attribute(attr);
            nodeRecord->append_node(node);
        }
        typedef list<Field>::const_iterator LI;
        for (LI i = record->fields.begin(); i != record->fields.end(); ++i)
        {
            Field f = *i;
            xml_node<> *nodeField = doc.allocate_node(node_element, "datafield");
            xml_attribute<> *attr = doc.allocate_attribute(
                "tag", string(f.tag).c_str());
            nodeField->append_attribute(attr);
            nodeRecord->append_node(nodeField);
            attr = doc.allocate_attribute(
                "ind1", doc.allocate_string(string(1,f.ind1).c_str()));
            nodeField->append_attribute(attr);
            attr = doc.allocate_attribute(
                "ind2", doc.allocate_string(string(1,f.ind2).c_str()));
            nodeField->append_attribute(attr);
            for (list<Subfield>::const_iterator j = f.subfields.begin();
                 j != f.subfields.end(); j++)
            {
                Subfield sf = *j;
                string value = sf.value;
                xml_node<> *node = doc.allocate_node(
                    node_element, "subfield", sf.value.c_str() );
                xml_attribute<> *attr = doc.allocate_attribute(
                    "code", doc.allocate_string(string(1,sf.letter).c_str()));
                node->append_attribute(attr);
                nodeField->append_node(node);
            }
        }

        *ost << doc;
        count++;
    }
};


class TextWriter : Writer {
private:
    std::ostream* ost;
    int count;
    char buffer[100000];

public:
    TextWriter(std::ostream* o) {
        ost = o;
        count = 0;
    }

    void operator<<( Record* record ) {
        char *p = buffer;
        string s = record->getLeader();
        s.copy(p, 24);
        p += 24;
        *p++ = '\n';
        for (list<CField>::const_iterator i = record->cfields.begin();
             i != record->cfields.end(); ++i)
        {
            CField f = *i;
            f.tag.copy(p, 3);
            p += 3;
            *p++ = ' ';
            f.value.copy(p, f.value.length());
            p += f.value.length();
            *p++ = '\n';
        }
        typedef list<Field>::const_iterator LI;
        for (LI i = record->fields.begin(); i != record->fields.end(); ++i)
        {
            Field f = *i;
            f.tag.copy(p, 3);
            p += 3;
            *p++ = ' ';
            *p++ = f.ind1;
            *p++ = f.ind2;            
            for (list<Subfield>::const_iterator j = f.subfields.begin();
                 j != f.subfields.end(); j++)
            {
                Subfield sf = *j;
                *p++ = ' ';
                *p++ = '$';
                *p++ = sf.letter;
                *p++ = ' ';
                sf.value.copy(p, sf.value.length());
                p += sf.value.length();
            }
            *p++ = '\n';
        }
        *p++ = '\n';

        ost->write(buffer, p-buffer);
        count++;
    }
};



}

