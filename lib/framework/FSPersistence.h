#ifndef FSPersistence_h
#define FSPersistence_h

#include <StatefulService.h>
#include <FS.h>

template <class T>
class FSPersistence 
{
public:
  FSPersistence(JsonStateReader<T> stateReader,
                JsonStateUpdater<T> stateUpdater,
                StatefulService<T>* statefulService,
                FS* fs,
                const char* filePath) 
    :
      _stateReader{stateReader},
      _stateUpdater{stateUpdater},
      _statefulService{statefulService},
      _fs{fs},
      _filePath{filePath},
      _updateHandlerId{0} 
  {
    enableUpdateHandler();
  }

  void readFromFS() 
  {
    File settingsFile = _fs->open(_filePath, "r");

    if (settingsFile)
    {
      JsonDocument json;
      DeserializationError error = deserializeJson(json, settingsFile);
      
      if (error == DeserializationError::Ok && json.is<JsonObject>()) 
      {
        JsonObject jsonObject = json.as<JsonObject>();
        _statefulService->updateWithoutPropagation(jsonObject, _stateUpdater);
        settingsFile.close();
        return;
      }

      settingsFile.close();
    }

    // If we reach here we have not been successful in loading the config and hard-coded defaults are now applied.
    // The settings are then written back to the file system so the defaults persist between resets. This last step is
    // required as in some cases defaults contain randomly generated values which would otherwise be modified on reset.
    applyDefaults();
    writeToFS();
  }

  bool writeToFS() 
  {
    // create and populate a new json object
    //DynamicJsonDocument jsonDocument = DynamicJsonDocument(_bufferSize);
    JsonDocument json;
    JsonObject jsonObject = json.to<JsonObject>();
    _statefulService->read(jsonObject, _stateReader);

    // make directories if required
    mkdirs();

    // serialize it to filesystem
    File settingsFile = _fs->open(_filePath, "w");

    // failed to open file, return false
    if (!settingsFile) {
      return false;
    }

    // serialize the data to the file
    serializeJson(json, settingsFile);
    settingsFile.close();
    return true;
  }

  void disableUpdateHandler() 
  {
    if (_updateHandlerId) 
    {
      _statefulService->removeUpdateHandler(_updateHandlerId);
      _updateHandlerId = 0;
    }
  }

  void enableUpdateHandler() 
  {
    if (!_updateHandlerId) 
    {
      _updateHandlerId = _statefulService->addUpdateHandler([&](const char* originId) { writeToFS(); });
    }
  }

 private:
  JsonStateReader<T> _stateReader;
  JsonStateUpdater<T> _stateUpdater;
  StatefulService<T>* _statefulService;
  FS* _fs;
  const char* _filePath;
  update_handler_id_t _updateHandlerId;

  // We assume we have a _filePath with format "/directory1/directory2/filename"
  // We create a directory for each missing parent
  void mkdirs() 
  {
    char path[128];  // Adjust size as needed
    strncpy(path, _filePath, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';  // Ensure null-termination
    
    char* p = path;
    while ((p = strchr(p + 1, '/')) != NULL) 
    {
      *p = '\0';  // Temporarily null-terminate the substring
      if (!_fs->exists(path)) {
        _fs->mkdir(path);
      }
      *p = '/';  // Restore the slash
    }
  }

protected:
  // We assume the updater supplies sensible defaults if an empty object
  // is supplied, this virtual function allows that to be changed.
  virtual void applyDefaults() 
  {
    JsonDocument json;
    JsonObject jsonObject = json.as<JsonObject>();
    _statefulService->updateWithoutPropagation(jsonObject, _stateUpdater);
  }
};

#endif  // end FSPersistence
