#include "OdroidExtra.h"
#include "genres.h"
#include <SD.h>

//#undef BOARD_HAS_PSRAM
//#define ROOT_GENRES "/_____gen.res"
//#define ROOT_GENRES "/g/en/r/e/s"

// If SD card used, claim SPI to avoid conflicting access to SPI
inline void claim_spi_genre(bool isSD) {if (isSD) claimSPI("genres");}            
inline void release_spi_genre(bool isSD) {if (isSD) claimSPI("false");}          

Genres genres("/_____gen.res/genres");

//extern void doprint ( const char* format, ... );


String GenreConfig::asJson() {
  String ret;
  const char *host = _rdbs?_rdbs:"de";
  ret = String(host);
  if ((0 == strcmp(host,"de")) || (0 == strcmp(host,"fr")) || (0 == strcmp(host,"nl")))
      ret = ret + "1.api.radio-browser.info";
  return  "{\"rdbs\": \"" + ret + "\"" +
              ",\"noname\": " + String(_noName?1:0) +
              ",\"showid\": " + String(_showId?1:0) +
              ",\"verbose\": " + String(_verbose?1:0) +
              ",\"isSD\":" + String(_genres->_isSD?1:0) +
              ",\"path\": \"" + String(_genres->_nameSpace) + "\"" +
              ",\"open\":" + String(_genres->_begun?1:0) +
               "}";
}



void GenreConfig::toNVS() {
    String value;
    nvssetstr("gcfg.rdbs", String(_rdbs));
//    nvssetstr("gcfg.usesd", String(_genres->_isSD?1:0));
//    nvssetstr("gcfg.path", String(_genres->_nameSpace));
    nvssetstr("gcfg.noname", String(_noName?1:0));
    nvssetstr("gcfg.verbose", String(_verbose?1:0));
    nvssetstr("gcfg.showid", String(_showId?1:0));
}

void GenreConfig::info() {
String infoLine;
    if (_genres)
    {
        const char *host = _rdbs?_rdbs:"de";                
        infoLine = "  gcfg.rdbs=" + String(host);
        _genres->dbgprint("Current genre configuration settings:");
        _genres->dbgprint(infoLine.c_str());
        infoLine = "  gcfg.usesd=" + String(_genres->_isSD?1:0);
        _genres->dbgprint(infoLine.c_str());
        infoLine = "  gcfg.path=" + String(_genres->_nameSpace);
        _genres->dbgprint(infoLine.c_str());
        infoLine = "  gcfg.noname=" + String(_noName?1:0);
        _genres->dbgprint(infoLine.c_str());
        infoLine = "  gcfg.verbose=" + String(_verbose?1:0);
        _genres->dbgprint(infoLine.c_str());
        infoLine = "  gcfg.showid=" + String(_showId?1:0);
        _genres->dbgprint(infoLine.c_str());
        _genres->dbgprint("(Note that these settings are not necessarily equal to preference settings in NVS.)");
        _genres->dbgprint("Use command \"gcfg.store\" to store the current settings to NVS.)");
    }
}



void GenreConfig::useSD(bool useSD) {
    if (_genres) 
        if (_genres->_isSD != useSD)
            {
            _genres->_isSD = useSD;
            if (_genres->_wasBegun)
                {
                _genres->_begun = false;
                _genres->begin();
                }
            }
}

Genres::Genres(const char *name) {
    if (NULL == name)
        name = "default";
    nameSpace(name);
    config._genres = this;
//    memcpy((void *)&_nameSpace, (void *)name, 8);
//    _nameSpace[8] = 0;
}

Genres::~Genres() {
    if (_nameSpace)
        free(_nameSpace);
}

void Genres::nameSpace(const char *name) {
    if (_nameSpace)
        free(_nameSpace);
    _isSD = false;
    if (name[2] == ':')
        if ((name[0] == 's')||(name[0] == 'S'))
            if (_isSD = (name[1] == 'd') || (name[1] == 'D'))
                name = name + 3;
    _nameSpace = strdup(name);
    if (_wasBegun)
    {
        _begun = false;
        this->begin();
    }
}

void Genres::dbgprint ( const char* format, ... ) {
#if !defined(NOSERIAL)
    if (config.verbose())
    {
        static char sbuf[2 * DEBUG_BUFFER_SIZE] ;                // For debug lines
        va_list varArgs ;                                    // For variable number of params
        sbuf[0] = 0 ;
        va_start ( varArgs, format ) ;                       // Prepare parameters
        vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
        va_end ( varArgs ) ;                                 // End of using parameters
        if ( DEBUG )                                         // DEBUG on?
        {
            Serial.print ( "D: GENRE: " ) ;                           // Yes, print prefix
        }
        else    
            Serial.print ("GENRE: ");
        Serial.println ( sbuf ) ;                            // always print the info
        Serial.flush();
    }
 #endif
}

/*
void Genres::verbose ( int mode ) {
#if !defined(NOSERIAL)
    _verbose = mode;
 #endif
}
*/

bool Genres::begin() {
    _wasBegun = true;
    if (_begun)                 // Already open?
        return true;
    /*
    if (_isSD)
        _fs = &SD;
    else
    */
        _fs = &LITTLEFS;
    if (_fs == &LITTLEFS)
    {
        if (!(_begun = LITTLEFS.begin()))
            {
            if (!(_begun = this->format()))
                {
                dbgprint("LITTLEFS formatting failed! Can not play from genres!");
                }
             }
        else
            _begun = openGenreIndex();
    }
    else    
        _begun = openGenreIndex();
    if (!_begun)
        _fs = NULL;
    return _begun;
}       

String Genres::fileName(const char *name) {
//    String ret = String(ROOT_GENRES) + String("/") + String(_nameSpace);
    String ret = String(_nameSpace);  
    if (name)
        ret = ret + "/" + name;
    return ret;
}

bool Genres::openGenreIndex(bool onlyAppend) {
bool res = false;
    //doprint("openGenreIndex started!")
    if (NULL != _psramList)
    {    
        free(_psramList);
        _psramList = NULL;
    }
    if (NULL != _gplaylist)
    {
        free(_gplaylist);
        _gplaylist = NULL;
    }
    if (!onlyAppend)
    {
        _cacheIdx = 0;
        memset(_idx, 0, sizeof(_idx));
    }
    uint16_t _lastKnownGenres = _knownGenres;
    _knownGenres = 0;
    if (!_fs)
        return false;
//    if (!(res = (0 == strlen(ROOT_GENRES))))
    if (!(res = (0 == strlen(_nameSpace))))
    {
//        char s[strlen(ROOT_GENRES) + 1];
//        strcpy(s, ROOT_GENRES);
        char s[strlen(_nameSpace) + 1];
        strcpy(s, _nameSpace);
        char *p = s;
        while (p) 
        {
            p = strchr(p + 1, '/');
            if (p) {
                *p = 0;
            }
            claim_spi_genre(_isSD);
            res = _fs->exists(s);
            release_spi_genre(_isSD);
            if (!res)
            {
                dbgprint("Creating ROOT-directory '%s' for genres", s);
                claim_spi_genre(_isSD);
                res = _fs->mkdir(s);
                release_spi_genre(_isSD);
            }
            if (!res)
                p = NULL;
            else if (p)
                *p = '/';
        }
        /*
        res = _fs->exists(ROOT_GENRES);
        if (!res)
        {
            dbgprint("Creating ROOT-directory '%s' for genres", ROOT_GENRES);
            res = _fs->mkdir(ROOT_GENRES);
        }
        */
    }
    /*
    if (res)
    {
        String fName = fileName();
        const char *s = fName.c_str();
        res = _fs->exists(s);
        if (!res)
        {
            dbgprint("Creating LITTLEFS directory '%s'", s);
            res = _fs->mkdir(s);
        }
    }
    */
    if (res)
    {
        dbgprint("LITTLEFS success opening directory '%s'", fileName().c_str());
        File idxFile;
        size_t size = 0;
        String fName = fileName("list");
        const char* s= fName.c_str();
        claim_spi_genre(_isSD);
        if ( _fs->exists(s) )
        {
         idxFile = _fs->open(s);
         release_spi_genre(_isSD);
         size = idxFile.size();
        }
        else
            release_spi_genre(_isSD);
        if (size > 0)
        {
            _psramList = (uint8_t *)gmalloc(idxFile.size());
            if (_psramList != NULL)
            {
                claim_spi_genre(_isSD);
                idxFile.read(_psramList, idxFile.size());
                idxFile.seek(0);
                release_spi_genre(_isSD);
            }
            size_t i = 0;
            int idx = 0;
            while (i < idxFile.size() && (idx < MAX_GENRES))
            {
                size_t skip; 
                claim_spi_genre(_isSD);
                idxFile.seek(i);
                skip = (size_t)idxFile.read();
                release_spi_genre(_isSD);
                if (!onlyAppend || (idx == _lastKnownGenres))
                {
                    _idx[idx].count = 0xffff;
                    _idx[idx++].idx = i++;
                }
                else 
                {
                    i++;
                    idx++;
                }
                if (skip >= 0)
                    i = i + skip;
            }
            _knownGenres = idx;
            if (onlyAppend)
            {
                cacheStep();
            }
        }
        else
            dbgprint("ListOfGenres file has size 0");
        claim_spi_genre(_isSD);
        idxFile.close();
        release_spi_genre(_isSD);
    }
    else
    {
        dbgprint("LITTLEFS could not create directory '%s'", fileName().c_str());
    }
    return res;
}

String Genres::getName(int id)
{
    char genreName[256];
    genreName[0] = 0;
    if (_fs && _begun && (id > 0) && (id <= _knownGenres))
    {
        id--;
        size_t idx = _idx[id].idx;
        uint16_t l = 0;
        if (_psramList)
        {   
            l = _psramList[idx];
            if (l < 256)
            {
                memcpy(genreName, _psramList + idx + 1, l);
                genreName[l] =  0;
            }
//            dbgprint("Got name from PSRAMLIST: %s", genreName);
        }
        else
        {
            claim_spi_genre(_isSD);
            File file = _fs->open(fileName("list").c_str(), "r");//"/genres/list", "r");
            file.seek(idx);
            l = file.read();
            if (l < 256) {
                file.read((uint8_t *)genreName, l);
                genreName[l] =  0;
            }
//            dbgprint("Got name from File: %s", genreName);
            file.close();
            release_spi_genre(_isSD);
        }
    }
    return String(genreName);
}

int Genres::findGenre(const char *s)
{
int res = -1;
int len = strlen(s);
    if (len > 255)
        len = 255;
    if (_fs && _begun && (len>0))
    {
        File idxFile;
        if (!_psramList)    
        {
            claim_spi_genre(_isSD);
            idxFile = _fs->open(fileName("list").c_str());//"/genres/list");
            release_spi_genre(_isSD);
        }
        res = 0;
        size_t idx = 0;
        for (int i = 0;i < _knownGenres;i++)
        {
            int l;
            if (_psramList) 
            {
                l = _psramList[idx++];
            }
            else
            {
                claim_spi_genre(_isSD);
                l = idxFile.seek(idx++);
                l = idxFile.read();
                release_spi_genre(_isSD);
            }
            if (l == len)
            {
                uint8_t genreName[l + 1];
                if (_psramList)
                    memcpy(genreName, _psramList + idx, l);
                else
                {
                    claim_spi_genre(_isSD);
                    idxFile.read(genreName, l);
                    release_spi_genre(_isSD);
                }
                genreName[l] = 0;
                if (0 == strcmp(s, (char *)genreName))
                {
                    res = i + 1;
                    break;
                }
            }
            idx = idx + l;
        }
        if (!_psramList)    
        {
            claim_spi_genre(_isSD);
            idxFile.close();
            release_spi_genre(_isSD);
        }
    }
    return res;
}

int Genres::createGenre(const char *s, bool deleteLinks)
{
int res;
    if (_fs == NULL)
        return 0;
    res  = findGenre(s);
    if (res < 0)
        res = 0;
    else if (res > 0)
        cleanGenre(res, deleteLinks);
    else if (_knownGenres < MAX_GENRES)
    {
        claim_spi_genre(_isSD);
        File idxFile = _fs->open(fileName("list").c_str(), "a");//"/genres/list", "a");
        release_spi_genre(_isSD);
        if (!idxFile)
        {
            dbgprint("Could not open '%s' for adding", fileName("list").c_str());
            res = 0;
        
        }
        else
        {
            bool valid;
            int l = strlen(s);
            if (l > 255)
                l = 255;
            dbgprint("Adding to idxFile, current size is %d", idxFile.size());
            claim_spi_genre(_isSD);
            if ( (valid = (idxFile.write((uint8_t)l) == 1)) )
                valid = (l == idxFile.write((const uint8_t *)s, l));
            idxFile.close();
            release_spi_genre(_isSD);
//            if (valid)
//                _knownGenres++;
//            else
            if (!valid)
            {
                res = 0;
                //openGenreIndex();
            }
            else if (openGenreIndex(true))
            {
                if ( res = findGenre(s) )
                    cleanGenre(res, true);
            }
        }
    }
    else
        res = 0;
    return res;
}


bool Genres::deleteGenre(int id)
{
bool ret = false;
    if (_fs && (id > 0) && (id < MAX_GENRES))
    {
        char path[30];
        if (_idx[id - 1].count != 0)
        {
            if (_idx[id - 1].count != 0xfffe)
                if (_gplaylist)
                {
                    free(_gplaylist);
                    _gplaylist = NULL;
                }
        }
        _idx[id - 1].count = 0;
        if (_cacheIdx == _knownGenres)
            {
              //force update of index file
              writeIndexFile();
            }

        sprintf(path, "%d/idx", id);
        const char* s = fileName(path).c_str();
        claim_spi_genre(_isSD);
        if ( (ret = _fs->exists(s)) )
        {
            _fs->remove(s);
            sprintf(path, "%d/urls", id);
            s = fileName(path).c_str();
            _fs->remove(s);
            sprintf(path, "%d/links", id);
            s = fileName(path).c_str();
            _fs->remove(s);
        }
        release_spi_genre(_isSD);
    }
    return ret;
}


bool Genres::addChunk(int id, const char *s, char delimiter) {
bool res = false;
    if (_fs && (id > 0) && (id <= _knownGenres) && (s != NULL))
        if (*s)
        {
        File urlFile, idxFile;
        uint16_t added = 0;
        char path[30];
        uint32_t count;
        String fnameStr;
        const char *fname;
            sprintf(path, "%d/idx", id);
            fnameStr = fileName(path);
            fname = fnameStr.c_str();
            claim_spi_genre(_isSD);
            idxFile = _fs->open(fname, "a");
            sprintf(path, "%d/urls", id);
            fnameStr = fileName(path);
            fname = fnameStr.c_str();
            urlFile = _fs->open(fname, "a");
            if (idxFile && urlFile)
            {
                res = true;
                size_t idx = urlFile.size();
                while(s)
                {
                    char *s1 = strchr(s, delimiter);
                    int l = s1?(s1 - s):strlen(s);
                    if (l > 0)
                    {
                        idxFile.write((uint8_t *)&idx, 4);
                        idx = idx + l + 1;
                        urlFile.write((uint8_t *)s, l);
                        urlFile.write(0);
                        added++;
                    }
                    if (s1)
                        s = s1 + 1;
                    else
                        s = NULL;
                }
            }
            idxFile.flush();
            count = idxFile.size() / 4;
            invalidateUrlCache(id);
            release_spi_genre(_isSD);
            if (count >= 0xffff)
                count = 0xfffe;
            _idx[id - 1].count = count;
            if (_knownGenres == _cacheIdx)
            {
              //Force update of index file 
              writeIndexFile();
            }
            if ((count == added) && (NULL != _gplaylist))
                addToPlaylist(id);
            claim_spi_genre(_isSD);
            idxFile.close();
            urlFile.close();
            release_spi_genre(_isSD);
        }
    return res;        
}   


#if defined(OBSOLETE)
bool Genres::add(int id, const char *s) {
    bool res = false;
    if (_fs && (id > 0) && (id <= _knownGenres))
    {
        dbgprint("Request to add URL '%s' to Genre with id=%d", (s?s:"<NULL>"), id);
        res = true;
        if (!s)
        {
            dbgprint("Can not add NULL-String!");
            res = false;
        }
        else if (!strlen(s))
        {
            dbgprint("Can not add empty URL-String!");
            res = false;
        }
        if (!res)
            return false;
        char path[50];
        const char* fname;
        sprintf(path, "%d/urls", id);
        String fnameStr = fileName(path);
        fname = fnameStr.c_str();
        claim_spi_genre(_isSD);
        File urlFile = _fs->open(fname, "a");
        release_spi_genre(_isSD);
        File idxFile;
        if (!urlFile)
        {
            dbgprint("Can not open file '%s' to append URL '%s'", fname, s);
            return false;
        }
        sprintf(path, "%d/idx", id);
        fname = fileName(path).c_str();
        claim_spi_genre(_isSD);
        idxFile = _fs->open(fname, "a");
        if (!idxFile)
        {
            urlFile.close();
            release_spi_genre(_isSD);
            dbgprint("Can not open index-file '%s'.", fname);
            return false;
        }
        size_t idx = urlFile.size();
//    _addIdx = _addIdx + strlen(s) + 1;
        urlFile.write((const uint8_t *)s, strlen(s) + 1);
        urlFile.close();
        idxFile.write((uint8_t *)&idx, sizeof(idx));
        idxFile.flush();
        release_spi_genre(_isSD);
        uint32_t count = idxFile.size() / 4;
        if (count < 0xfffe)
            count++;
        else
            count = 0xfffe;
        if ((count == 1) && (_gplaylist))
            addToPlaylist(id);
        _idx[id - 1].count = count;
        claim_spi_genre(_isSD);
        idxFile.close();
        release_spi_genre(_isSD);
    }
    return true;
}
#endif

bool Genres::deleteAll() {
    if (_fs)
    {
        deleteDir(fileName().c_str());
        if ( !(_begun = openGenreIndex()) )
            _fs = NULL;
    }
    return _fs;
}

void Genres::ls() {
    listDir(fileName().c_str());    
}

void Genres::test() {
    if (_fs == &LITTLEFS)
    dbgprint("Genre Filesystem total bytes: %ld, used bytes: %ld (free=%ld)",
        LITTLEFS.totalBytes(), LITTLEFS.usedBytes(), LITTLEFS.totalBytes() - LITTLEFS.usedBytes());
    else if (_fs)
        dbgprint("No information available for used Genre Filesystem!");
    else  
        dbgprint("Genre Filesystem is not mounted!");
}


String Genres::playList() {
    String res;
    
    if (_gplaylist != NULL)
        res = String(_gplaylist);
    else
    {
        bool notFirst = false;
        Serial.println("Building genre playlist from scratch!");
        for(int i = 1;i <= _knownGenres;i++)
            if (count(i) > 0)
            {
                if (notFirst)
                    res = res + "," + getName(i);
                else
                {
                    res = getName(i);
                    notFirst = true;
                }
            }
        if ( (_gplaylist = (char *)gmalloc(res.length() + 1)) )
            memcpy(_gplaylist, res.c_str(), res.length() + 1);
    }
    return res;
}

void Genres::listDir(const char * dirname){
    dbgprint("Listing directory: %s", dirname);

    if (!_fs)
    {
        dbgprint("Genre Filesystem is not mounted!");
        return;
    }
    claim_spi_genre(_isSD);
    File root = _fs->open(dirname);
    release_spi_genre(_isSD);
    if(!root)
    {
        dbgprint("- failed to open directory");
        return;
    }
    if(!root.isDirectory())
    {
        dbgprint(" - not a directory");
        return;
    }
    claim_spi_genre(_isSD);
    File file = root.openNextFile();
    release_spi_genre(_isSD);
    String rootDir = fileName("");
    const char* rootDirName = rootDir.c_str();
    int rootDirLen = strlen(rootDirName);
    while(file)
    {
        if(file.isDirectory())
        {
            const char *s;
            if ( (s = strstr(file.name(), rootDirName)) )
                s = s + rootDirLen;
            else
                s = "0";
            dbgprint("  DIR : %s (GenreName[%s]: %s)", file.name(), s, getName(atoi(s)).c_str());
            listDir(file.name());    
        } 
        else 
        {
            dbgprint("  FILE: %s\tSize: %ld", file.name(), file.size());
        }
        claim_spi_genre(_isSD);
        file = root.openNextFile();
        release_spi_genre(_isSD);
    }
}

void Genres::deleteDir(const char * dirname){
char s[100];
    dbgprint("Deleting directory: %s", dirname);
    if (!_fs)
    {
        dbgprint("Genre Filesystem is not mounted!");
        return;
    }
    claim_spi_genre(_isSD);
    File root = _fs->open(dirname);
    release_spi_genre(_isSD);
    if(!root)
    {
        dbgprint("- failed to open directory");
        return;
    }
    if(!root.isDirectory())
    {
        dbgprint(" - not a directory");
        return;
    }
    claim_spi_genre(_isSD);
    File file = root.openNextFile();
    release_spi_genre(_isSD);
    while(file)
    {
        strcpy(s, file.name());
        bool isDir = file.isDirectory();
        claim_spi_genre(_isSD);
        file = root.openNextFile();
        release_spi_genre(_isSD);
        if(isDir)
        {
            deleteDir(s);    
        } 
        else 
        {
            dbgprint("Delete file '%s'", s);
            claim_spi_genre(_isSD);
            _fs->remove(s);
            release_spi_genre(_isSD);
        }
    }
    claim_spi_genre(_isSD);
    _fs->rmdir(dirname);
    release_spi_genre(_isSD);
}



void Genres::lsJson(Print& client, uint8_t lsmode)
{
    client.print("[");
    for(int id=1, total=0;id <= _knownGenres;id++)
    {
        if ( count(id) > 0 )
        {
            if (total++)
                client.print(',');
            client.println();
            client.printf("{\"id\": %d, \"name\": \"%s\"", id, getName(id).c_str());

            if (0 == (lsmode & LSMODE_SHORT))
            {
                uint16_t numUrls = count(id);
                client.printf(", \"presets\": %d, \"mode\": \"<valid>\"", numUrls);
            }
            if (0 != (lsmode & LSMODE_WITHLINKS))
            {
                String links = getLinks(id);
                client.printf(", \"links\": \"%s\"", links.c_str());
            }
            client.print('}');
        }
    }
    client.println();
    client.println("]");
}

uint16_t Genres::count(int id) {
uint32_t res = 0;
//char path[30];
String fnameStr;
const char *fname;

    if (_fs && (id > 0) && (id <= _knownGenres))
    {
        if ((_cacheIdx == 0) && (id == 1))
        {
            String fnameStr = fileName("idx");
            const char *fname = fnameStr.c_str();
            claim_spi_genre(_isSD);
            if ( _fs->exists(fname) )
            {
                File file = _fs->open(fname, "r");
                release_spi_genre(_isSD);
                size_t fSize = file.size();
                res = fSize / sizeof(_idx[0]);
                if (res == _knownGenres)
                {
                    claim_spi_genre(_isSD);
                    file.read((uint8_t *)&_idx, sizeof(_idx[0]) * _knownGenres);
                    file.close();
                    release_spi_genre(_isSD);
                    dbgprint("Idx cache file opened (%s)", fname); 
                    _cacheIdx = _knownGenres;
                }
                else
                {
                    claim_spi_genre(_isSD);
                    file.close();
                    _fs->remove(fname);
                    dbgprint("Idx cache file corrupt (%s), has been removed!", fname); 
                    release_spi_genre(_isSD);
                }
            }
            else
                release_spi_genre(_isSD);
        }
        if (_idx[id-1].count != 0xffff)
        {
            res = _idx[id - 1].count;
            dbgprint("Got count for id: %d from cached idx-File as: %d (CacheIdx=%d)", id, res, _cacheIdx);
        }
        else
        {
            char pathId[20];
            sprintf(pathId, "%d/idx", id);
            fnameStr = fileName(pathId);
            fname = fnameStr.c_str();
            claim_spi_genre(_isSD);
            if ( _fs->exists(fname) )
            {
                File file = _fs->open(fname, "r");
                size_t fSize = file.size();
                res = fSize / 4;
                file.close();
                release_spi_genre(_isSD);
            }
            else
                release_spi_genre(_isSD);
            if (res <= 0xfffe)
                _idx[id - 1].count = res;
            else
                res = _idx[id - 1].count = 0xfffe;
            dbgprint("Calculated count for id: %d as: %d", id, res);

        }
    //force writeout of index file
      if (_cacheIdx == id - 1)
      {
        dbgprint("CacheIDX=%d is id-1, KnownGenres = %d", _cacheIdx, _knownGenres);
        _cacheIdx = id;
        if (id == _knownGenres)
        {
          writeIndexFile();
        }
      }
    }
    


    if (res >= 0xfffe)
        res = 0;
    return res;
}


void Genres::writeIndexFile()
{
          String fnameStr = fileName("idx");
          const char *fname = fnameStr.c_str();
          claim_spi_genre(_isSD);
          File file = _fs->open(fname, "w");
          file.write((uint8_t *)&_idx, sizeof(_idx[0]) * _knownGenres);
          file.close();
          release_spi_genre(_isSD);
          dbgprint("Updated idx-File on filesystem: %s", fname);
}

bool Genres::cacheStep()
{
    if (_knownGenres > _cacheIdx) {
        count(_cacheIdx + 1);
        if (((_cacheIdx % 10) == 0) || (_cacheIdx == _knownGenres)) 
            dbgprint("CacheStep: %d (knownGenres: %d)", _cacheIdx, _knownGenres);
    }
  return _knownGenres == _cacheIdx;
}

String Genres::getUrl(int id, uint16_t number, bool cutName) {
String res = "";
    
    if (_fs && (count(id) > number))
    {
        File fileIdx, fileUrls;
        size_t seekPosition = number;
        size_t chunkStart;
        size_t chunkSize, idxSize, urlSize;
        seekPosition = seekPosition * 4;
        uint8_t *idxCache;
        uint8_t *urlCache = getUrlCache(id, urlSize, fileUrls, fileIdx, &idxCache, idxSize);
        if (NULL != idxCache)
        {
          idxCache = idxCache + seekPosition;
          memcpy(&chunkStart, idxCache, 4);
          if (seekPosition + 4 < idxSize)
          {
            memcpy(&chunkSize, idxCache + 4, 4);
          }
            else
                chunkSize = urlSize;
        }
        else if (fileIdx)
        {
          claim_spi_genre(_isSD);
          fileIdx.seek(seekPosition);
          fileIdx.read((uint8_t *)&chunkStart, 4);
          if (seekPosition + 4 < fileIdx.size())
          {
              fileIdx.read((uint8_t *)&chunkSize, 4);
          }
          else
              chunkSize = urlSize;
          fileIdx.close();
          release_spi_genre(_isSD);
        }
        dbgprint("About to seek: %ld..%ld (urlSize: %ld) fileUrls=%d", chunkStart, chunkSize, urlSize, fileUrls);
        if ((chunkSize > chunkStart) && (chunkSize <= urlSize) && (fileUrls || (NULL != urlCache)))
        {
          // chunkSize now contains the start index of next chunk, make it chunkSize with next expression
          chunkSize = chunkSize - chunkStart;
          char *s = (char *)malloc(chunkSize);
          if (s)
          {
            dbgprint("Ready to locate url in %s, chunkStart=%ld, chunkSize=%ld, idx=%d", 
                        (urlCache?"cache":"file"), chunkStart, chunkSize, number);
            if (NULL != urlCache)
            {
              urlCache = urlCache + chunkStart;
              memcpy(s, urlCache, chunkSize);
            }
            else   // not cached, get URL from file.
            {
                claim_spi_genre(_isSD);
                fileUrls.seek(chunkStart);
                fileUrls.read((uint8_t *)s, chunkSize);
                fileUrls.close();
                release_spi_genre(_isSD);
            }
            char *p;
            if (cutName)
              if (NULL != (p = strchr(s, '#')))
                *p = 0;
            res = String(s);
            free(s);
          }
        }
        if (fileUrls)  
        {
          claim_spi_genre(_isSD);
          fileUrls.close();
          release_spi_genre(_isSD);
        }
    }
    return res;
}

#if defined(OBSOLETE)    
        char path[20];
        sprintf(path, "%d/idx", id);
        claim_spi_genre(_isSD);
        fileIdx = _fs->open(fileName(path).c_str(), "r");
        sprintf(path, "%d/urls", id);
        fileUrls = _fs->open(fileName(path).c_str(), "r");
        if (fileIdx && fileUrls)
        {
            size_t seekPosition = number * 4;
            size_t chunkStart;
            size_t chunkSize;
            fileIdx.seek(seekPosition);
            fileIdx.read((uint8_t *)&chunkStart, 4);
            if (seekPosition + 4 < fileIdx.size())
            {
                fileIdx.read((uint8_t *)&chunkSize, 4);
            }
            else
                chunkSize = fileUrls.size();
            release_spi_genre(_isSD);

            // chunkSize now contains the start index of next chunk, make it chunkSize with next expression
            //number = number % URL_CHUNKSIZE;
            chunkSize = chunkSize - chunkStart;
            dbgprint("Ready to locate url in file, chunkStart=%ld, chunkSize=%ld, idx=%d", chunkStart, chunkSize, number);
            char *s = (char *)malloc(chunkSize);
            if (s)
            {
                claim_spi_genre(_isSD);
                fileUrls.seek(chunkStart);
                fileUrls.read((uint8_t *)s, chunkSize);
                release_spi_genre(_isSD);
                char *p;
                if (cutName)
                    if (NULL != (p = strchr(s, '#')))
                        *p = 0;
                res = String(s);
                free(s);
            }
        }
        claim_spi_genre(_isSD);
        fileIdx.close();
        fileUrls.close();
        release_spi_genre(_isSD);
   
    }
    return res;

}
#endif 


void Genres::cleanLinks(int id){
    if (_fs && _begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        File linkFile;
        sprintf(path, "%d/links", id);
        claim_spi_genre(_isSD);
        linkFile = _fs->open(fileName(path).c_str(), "w");
        if (linkFile)
            linkFile.close();
        release_spi_genre(_isSD);
    }
}

void Genres::addLinks(int id, const char* moreLinks){
    if (_fs && _begun && (id > 0) && (id <=_knownGenres) && (strlen(moreLinks)))
    {
        char path[30];
        File linkFile;
        sprintf(path, "%d/links", id);
        claim_spi_genre(_isSD);
        linkFile = _fs->open(fileName(path).c_str(), "a");
        if (linkFile)
        {
            if (linkFile.size() > 0)
                linkFile.write(',');
            linkFile.write((uint8_t *)moreLinks, strlen(moreLinks));
            linkFile.close();
            release_spi_genre(_isSD);    
            dbgprint("Add link to genre[%d]=%s (len=%d) (filesize so far: %ld", 
                    id, moreLinks, strlen(moreLinks), linkFile.size());

        }
        else
            release_spi_genre(_isSD);
    }
}

String Genres::getLinks(int id) {
String ret ="";
    if (_fs && _begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        sprintf(path, "%d/links", id);
        claim_spi_genre(_isSD);
        if ( _fs->exists(fileName(path).c_str() ))
        {
            File linkFile = _fs->open(fileName(path).c_str(), "r");
            char s[linkFile.size() + 1];
            linkFile.read((uint8_t *)s, linkFile.size());
            s[linkFile.size()] = 0;
            linkFile.close();
            release_spi_genre(_isSD);
            ret = String(s);
            dbgprint("Returning links for genre[%d]=%s", id, ret.c_str());
        }
            release_spi_genre(_isSD);
    }
    return ret;
}

void Genres::cleanGenre(int id, bool deleteLinks)
{
    String fnameStr;
    const char *fname;
    if (_begun && _fs)
    {
    char s0[20];
    char s[50];
        if (_idx[id - 1].count > 0)
        {
            if (_gplaylist)
            {
                free(_gplaylist);
                _gplaylist = NULL;
            }
            _idx[id - 1].count = 0;
            if (_cacheIdx == _knownGenres)
            {
              writeIndexFile();
              //force update of index file
            }
            Serial.println("gplaylist deleted!");
        }
        //TODO delete specific files for Genre
        sprintf(s0, "%d", id);
        fnameStr = fileName(s0);
        fname = fnameStr.c_str();
        claim_spi_genre(_isSD);
        File root = _fs->open(fname);
        release_spi_genre(_isSD);
        if (!root) {
            dbgprint("No content is stored in LITTLEFS for genre with id: %d", id);
            dbgprint("Creating directory %s", fname);
            claim_spi_genre(_isSD);
            _fs->mkdir(fname);
            release_spi_genre(_isSD);
        }
        else if (!root.isDirectory()) {
            dbgprint("PANIC because unexpected: File with name %s is not a directory!", fname);
            claim_spi_genre(_isSD);
            root.close();
            release_spi_genre(_isSD);
            return;
        }
        else
        {
            claim_spi_genre(_isSD);
            File file = root.openNextFile();
            release_spi_genre(_isSD);
            while (file)
            {
                char *s = strdup(file.name());
                claim_spi_genre(_isSD);
                file = root.openNextFile();
                release_spi_genre(_isSD);
                if (deleteLinks || (strstr(s, "links") == NULL))
                    dbgprint("Removing '%s'=%d", s, _fs->remove(s));
                free(s);
            }        
            //dbgprint("Removing directory '%s'=%d", s0, LITTLEFS.rmdir(s0));
            claim_spi_genre(_isSD);
            root.close();
            release_spi_genre(_isSD);
        }
        
       //????? root = _fs->open(fname); // Directory should be open now..
       
        /*
        if (root && (root.isDirectory()))
        {

            sprintf(s, "%s/idx", s0);
            File file = _fs->open(s, "w");
            if (file)
            {
//                uint8_t buf[5];
//                memset(buf, 0, 5);
//                file.write(buf, 5);
                file.close();
            }
            sprintf(s, "%s/urls", s0);
            file = _fs->open(s, "w");
            if (file)
            {
                file.close();
            }
        }
        else
        {
            dbgprint("PANIC because unexpected: Directory with name %s not found!", s0);
        }
        */
        //????? root.close();

    }
    else 
        dbgprint("LITTLEFS is not mounted (for deleting genre with id: %d", id);
}

bool Genres::format(bool quick)
{
bool ret = false;
    if (_fs == &LITTLEFS)
    {
        if (quick)
        {
            File f;
            dbgprint("Quick Formatting LITTLEFS for genre!");
            //??? Filename!!!!
            f = _fs->open("/genres/list", "w");
            ret = f;
            f.close();
            if (ret)
                _begun = ret = openGenreIndex();
        }
        else
        { 
            dbgprint("Formatting LITTLEFS for genre, that might take some time!");
            if (LITTLEFS.format())
                if (LITTLEFS.begin())
                    _begun = ret = openGenreIndex();
        }
    }
    else if (_fs)
    {
        dbgprint("Deleting everything from filesystem for genre, that might take some time!");
        ret = deleteAll();
    }
    if (ret)
        dbgprint("Success formatting LITTLEFS!");
    else
        dbgprint("Error formatting LITTLEFS!");
    return ret;
}


void *Genres::gmalloc(size_t size, bool forcePSRAM)
{
    void *p = NULL;           
#if defined(BOARD_HAS_PSRAM)
    p = (uint8_t *)ps_malloc(size);
#endif
    if (!forcePSRAM)
      if (!p)
        if (ESP.getFreeHeap() > size)
            if (ESP.getFreeHeap() - size > 80000)
                p = malloc(size);
    return p;
}

void Genres::addToPlaylist(int idx){
    if ((idx > 0) && (idx < _knownGenres) && (NULL != _gplaylist))
    {
        String name = getName(idx);
        if (name.length() > 0)
        {
            if (_gplaylist[0] != 0)
                name = "," + name;
            char *p = (char *)gmalloc(name.length() + strlen(_gplaylist) + 2);
            if (p)
            {
                strcpy(p, _gplaylist);
                strcat(p, name.c_str());
                free(_gplaylist);
                _gplaylist = p;
                Serial.printf("Added new genre to _gplaylist: %s\r\n", _gplaylist);
            }
            else
            {
                free(_gplaylist);
                _gplaylist = NULL;

            }

        }
    }

}

uint8_t * Genres::getUrlCache(int id, size_t &urlSize, File& fileUrls, File& fileIdx, uint8_t **idxCache, size_t &idxSize)
{
char path[20];

  *idxCache = NULL;
  if (0 == count(id))
    return NULL;
  if (_urlCache)
    if (_urlId == id)
    {
      urlSize = _urlSize;
      if (_idxCache)
      {
        *idxCache = _idxCache;
        idxSize = _idxSize;
      }
      else
      {
        sprintf(path, "%d/idx", id);
        claim_spi_genre(_isSD);
        fileIdx = _fs->open(fileName(path).c_str(), "r");        
        release_spi_genre(_isSD);
        idxSize = fileIdx.size();  
      }
      return _urlCache;          
    }
    else
      invalidateUrlCache(-1);
  sprintf(path, "%d/idx", id);
  claim_spi_genre(_isSD);
  fileIdx = _fs->open(fileName(path).c_str(), "r");        
  sprintf(path, "%d/urls", id);
  fileUrls = _fs->open(fileName(path).c_str(), "r");        
  release_spi_genre(_isSD);
  idxSize = fileIdx.size();  
  urlSize = fileUrls.size();  
  if (fileUrls && fileIdx && urlSize && idxSize)
  {
    uint32_t startTime = millis();
    _urlCache = (uint8_t *)gmalloc(urlSize);
    if (NULL != _urlCache)
    {
      claim_spi_genre(_isSD);
      fileUrls.read(_urlCache, urlSize);
      fileUrls.close();
      _urlSize = urlSize;
      if (NULL != (_idxCache = (uint8_t *)gmalloc(idxSize)))
      {
        fileIdx.read(_idxCache, idxSize);
        fileIdx.close();
        *idxCache = _idxCache;
        _idxSize = idxSize;
      }
      release_spi_genre(_isSD);       
      _urlId = id;
      startTime = millis() - startTime;
      dbgprint("Cached genre %d: UrlFileSize = %ld (IdxFileSize = %ld), CPU-Time: %ld",  id, _urlSize, _idxSize, startTime);
    }
  }
  else
  {
    dbgprint("Closing because UrlSize (open: %d): %ld, IdxSize (open: %d): %ld", fileUrls?1:0, urlSize, fileIdx?1:0, idxSize);
    claim_spi_genre(_isSD);
    fileIdx.close();
    fileUrls.close();
    release_spi_genre(_isSD);
    idxSize = urlSize = 0;
  }
  return _urlCache;
}
        
void Genres::invalidateUrlCache(int id)
{
  if (_urlCache)
    if ((_urlId == id) || (id == -1))
    {
      free(_urlCache);
      _urlCache = NULL;
      if (_idxCache)
        _idxCache = NULL;
      _urlId = -1;
      _idxSize = 0;
      _urlSize = 0;
    }
}
