#include "Variant.hpp"

#include "fileformats/Bed.hpp"

#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <sstream>
#include <sstream>
#include <stdexcept>

using boost::format;
using namespace std;

string Variant::typeToString(Type t) {
    switch (t) {
    case SNP:
        return "SNP";
        break;

    case INS:
        return "INS";
        break;

    case DEL:
        return "DEL";
        break;

    default:
        return "INVALID";
        break;
    }
}

Variant::Type Variant::inferType() const {
    if (_stop  == _start+1 && !reference().null() && !variant().null())
        return SNP;
    else if (_stop == _start && reference().null() && !variant().null())
        return INS;
    else if (_stop != _start && variant().null())
        return DEL;
    else {
        stringstream ss;
        ss << "Could not determine type of variant: ";
        toStream(ss);
        throw runtime_error(ss.str());
    }
}

Variant::Variant() : _type(INVALID) {
    _allSequences.push_back(Sequence("-"));
    _allSequences.push_back(Sequence("-"));
}

Variant::Variant(const Bed& bed)
    : _chrom(bed.chrom())
    , _start(bed.start())
    , _stop (bed.stop())
    , _quality(0)
    , _depth(0)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("/", "", boost::keep_empty_tokens);
    tokenizer tokens(bed.extraFields()[0], sep);
    for (tokenizer::iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
        _allSequences.push_back(*iter);
    while (_allSequences.size() < 2)
        _allSequences.push_back(Sequence("-"));

    if (bed.extraFields().size() >= 2) {
        stringstream ss(bed.extraFields()[1]);
        ss >> _quality;
        if (ss.fail())
            throw runtime_error(str(format("Failed converting quality value '%1%' to number for record '%2%'") %bed.extraFields()[1] %bed.toString()));
    }

    if (bed.extraFields().size() >= 3) {
        stringstream ss(bed.extraFields()[2]);
        ss >> _depth;
        if (ss.fail())
            throw runtime_error(str(format("Failed converting read depth value '%1%' to number for record '%2%'") %bed.extraFields()[2] %bed.toString()));
    }

    _type = inferType();
}

ostream& Variant::toStream(ostream& s) const {
    s << chrom() << "\t" <<
        start() << "\t" <<
        stop() << "\t" <<
        reference().data() << "\t" <<
        variant().data() << "\t" <<
        typeToString(type());

    return s;
}

bool Variant::allelePartialMatch(const Variant& rhs) const {
    if (reference() != rhs.reference())
        return false;

    return iubOverlap(variant().data(), rhs.variant().data());
}

namespace {
    bool doDbSnpMatchHack(const Variant& a, const Variant& dbSnp) {

        // this is nuts
        // we can't trust dbsnp data, so we have to try each allele as the
        // reference, and the combination of all the // rest as the variant.
        // if that doesn't work, we reverse complement and do it again.
        for (unsigned i = 0; i < dbSnp.allSequences().size(); ++i) {

            const Sequence& ref(dbSnp.allSequences()[i]);
            if (ref != a.reference())
                continue;

            unsigned allelesBin = 0;
            for (unsigned j = 0; j < dbSnp.allSequences().size(); ++j) {
                if (j == i) continue; // skip the one we picked as reference
                allelesBin |= alleles2bin(dbSnp.allSequences()[j].data().c_str());
            }
            string iub(1, alleles2iub(allelesBin));
            if (iubOverlap(a.variant().data(), iub))
                return true;
        }
        return false;
    }
}

bool Variant::alleleDbSnpMatch(const Variant& dbSnp) const {

    if (doDbSnpMatchHack(*this, dbSnp))
        return true;

    // didn't match? reverse complement dbsnp and try again;
    Variant revDbSnp(dbSnp);
    revDbSnp.reverseComplement();
    return doDbSnpMatchHack(*this, revDbSnp);
}