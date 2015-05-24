#ifndef AAALGO_DONKEY_COMMON
#define AAALGO_DONKEY_COMMON

#include <array>
#include <algorithm>
#include <iostream>
#include <boost/type_traits.hpp>
// common feature and objects 

namespace donkey {
    // utility functions
    static inline void ReadFile (const std::string &path, std::string *binary) {
        // file could not be too big
        binary->clear();
        ifstream is(path.c_str(), std::ios::binary);
        if (!is) return;
        is.seekg(0, std::ios::end);
        size_t size = size_t(is.tellg());
        if (size > MAX_BINARY) return;
        binary->resize(size);
        if (size) {
            is.seekg(0, std::ios::beg);
            is.read((char *)&binary->at(0), size);
        }
        if (!is) binary->clear();
    }

    static inline void WriteFile (const std::string &path, std::string const &binary) {
        // file could not be too big
        ofstream os(path.c_str(), ios::binary);
        os.write(&binary[0], binary.size());
    }

    struct ObjectBase {
        string meta;
    };

    // Feature extractor interface.
    template<class Object>
    class ExtractorBase {
        public:
            virtual ~ExtractorBase () {}
            virtual void extract_path (string const &path, Object *object) const {
                string content;
                ReadFile(path, &content);
                extract(content, object);
            }
            virtual void extract (string const &content, Object *object) const; 
    };

    struct PositiveSimilarity {
        static constexpr int POLARITY = 1;
    };

    struct NegativeSimilarity {
        static constexpr int POLARITY = -1;
    };

    template <typename T, unsigned D>
        struct VectorFeature {
            typedef T value_type;
            static unsigned constexpr DIM = D;
            std::array<T, D> data;

            void read (std::istream &is) {
                is.read(reinterpret_cast<char *>(&data[0]), sizeof(value_type) * DIM);
            }

            void write (std::ostream &os) const {
                os.write(reinterpret_cast<char const *>(&data[0]), sizeof(value_type) * DIM);
            }
        };

    namespace distance {

        typedef NegativeSimilarity Distance;

        template <typename T, unsigned D>
            struct L1: public Distance {
                typedef VectorFeature<T,D> feature_type;
                static float apply (feature_type const &v1, feature_type const &v2) {
                    double v = 0;
                    for (unsigned i = 0; i < D; ++i) {
                        double a = std::abs(v1.data[i] - v2.data[i]);
                    }
                    return v;
                }
            };

        template <typename T, unsigned D>
            struct L2: public Distance {
                typedef VectorFeature<T,D> feature_type;
                static float apply (feature_type const &v1, feature_type const &v2) {
                    double v = 0;
                    for (unsigned i = 0; i < D; ++i) {
                        double a = v1.data[i] - v2.data[i];
                        v += a * a;
                    }
                    return std::sqrt(v);
                }
            };

        template <typename T, unsigned D>
            struct TypeHamming: public Distance {
                typedef VectorFeature<T, D> feature_type;
                static float apply (feature_type const &v1, feature_type const &v2) {
                    int v = 0;
                    for (unsigned i = 0; i < D; ++i) {
                        if (v1.data[i] != v2.data[i]) {
                            ++v;
                        }
                    }
                    return v;
                }
            };
    }

    template <typename T>
        struct SingleFeatureObject: public ObjectBase {
            typedef T feature_type;
            feature_type feature;
            void enumerate (function<void(unsigned tag, feature_type const *)> callback) const {
                callback(0, &feature);
            }

            void read (std::istream &is) {
                feature.read(is);
            }

            void write (std::ostream &os) const {
                feature.write(os);
            }

            void swap (SingleFeatureObject<T> &v) {
                std::swap(feature, v.feature);
            }
        };

    template <typename T, typename D = void>
        struct MultiPartObject: public ObjectBase {
            typedef T feature_type;
            typedef D data_type;
            struct Part {
                feature_type feature;
                data_type data;
            };
            vector<Part> parts;


            void enumerate (function<void(unsigned tag, feature_type const *)> callback) const {
                for (unsigned i = 0; i < parts.size(); ++i) {
                    callback(i, &parts[i].feature);
                }
            }

            void read (std::istream &is) {
                uint16_t sz;
                is.read(reinterpret_cast<char *>(&sz), sizeof(sz));
                BOOST_VERIFY(sz <= MAX_FEATURES);
                parts.resize(sz);
                for (auto &part: parts) {
                    if (!boost::is_void<data_type>::value) {
                        is.read(reinterpret_cast<char *>(&part.data), sizeof(data_type));
                    }
                    part.feature.read(is);
                }
            }

            void write (std::ostream &os) const {
                uint16_t sz = parts.size();
                os.write(reinterpret_cast<char const *>(&sz), sizeof(sz));
                for (auto const &part: parts) {
                    if (!boost::is_void<data_type>::value) {
                        os.write(reinterpret_cast<char const *>(&part.data), sizeof(data_type));
                    }
                    part.feature.write(os);
                }
            }

            void swap (MultiPartObject<T, D> &v) {
                std::swap(parts, v.parts);
            }
        };

    template <typename T>
        class TrivialMatcher {
            public:
                typedef T feature_similarity_type;
                static constexpr int POLARITY = feature_similarity_type::POLARITY;

                TrivialMatcher (Config const &config) {
                }

                float apply (Object const &query, Candidate const &cand) const {
                    BOOST_VERIFY(cand.hints.size());
                    return cand.hints[0].value;
                }
        };

    template <typename T>
        class CountingMatcher {
            float th;
            public:
            typedef T feature_similarity_type;
            static constexpr int POLARITY = 1;

            CountingMatcher (Config const &config):
                th(config.get<float>("donkey.matcher.threshold") * POLARITY)
            {
            }

            float apply (Object const &query, Candidate const &cand) const {
                BOOST_VERIFY(cand.hints.size());
                unsigned v = 0;
                for (auto const &h: cand.hints) {
                    if (h.value * POLARITY >= th) {
                        ++v;
                    }
                }
                return v;
            }
        };
}

#endif
