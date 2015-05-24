#include "donkey-common.h"
namespace donkey{
    void ExtractorBase::extract (string const &content, Object *object) const { 
        namespace fs = boost::filesystem;
        fs::path path(fs::unique_path());
        WriteFile(path.native(), content);
        extract_path(path.native(), object);
        fs::remove(path);
    }
}
