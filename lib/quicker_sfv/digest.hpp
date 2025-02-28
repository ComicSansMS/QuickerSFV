#ifndef INCLUDE_GUARD_QUICKER_SFV_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_DIGEST_HPP

#include <concepts>
#include <memory>
#include <string>

namespace quicker_sfv {

template<typename T>
concept IsDigest = std::regular<T> && requires(T const d) {
    { d.toString() } -> std::same_as<std::u8string>;
};

class Digest {
private:
    class Concept {
    public:
        virtual ~Concept() = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual std::u8string toString() const = 0;
        virtual bool equalTo(Concept const& rhs) const = 0;
        virtual std::type_info const& getType() const = 0;
    };

    template<IsDigest T>
    class Model: public Concept {
    private:
        T m_;
    public:
        explicit Model(T const& m)
            :m_(m)
        {}

        explicit Model(T&& m)
            :m_(std::move(m))
        {}

        ~Model() override = default;

        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(m_);
        }

        std::u8string toString() const override {
            return m_.toString();
        }

        bool equalTo(Concept const& rhs) const override {
            Model const* mrhs = dynamic_cast<Model const*>(&rhs);
            if (!mrhs) { return false; }
            return m_ == (mrhs->m_);
        }

        std::type_info const& getType() const {
            return typeid(T);
        }
    };

    std::unique_ptr<Concept> m_digest;
public:
    template<typename T> requires( !std::is_same_v<std::remove_cvref_t<T>, Digest> ) && IsDigest<T>
    Digest(T&& digest)
        :m_digest(std::make_unique<Model<std::remove_cvref_t<T>>>(std::forward<T>(digest)))
    {}

    Digest();
    ~Digest();
    Digest(Digest const& rhs);
    Digest& operator=(Digest const& rhs);
    Digest(Digest&& rhs);
    Digest& operator=(Digest&& rhs);

    [[nodiscard]] std::u8string toString() const;

    bool checkType(std::type_info const& ti) const;

    friend bool operator==(Digest const& lhs, Digest const& rhs);
    friend bool operator!=(Digest const& lhs, Digest const& rhs);
};

static_assert(std::regular<Digest>, "Digest is not regular");

}
#endif
