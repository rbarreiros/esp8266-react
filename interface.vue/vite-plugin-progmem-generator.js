import { resolve, relative, sep } from 'path';
import { readdirSync, existsSync, unlinkSync, readFileSync, createWriteStream } from 'fs';
import zlib from 'zlib';
import mime from 'mime-types';

const ARDUINO_INCLUDES = "#include <Arduino.h>\n\n";

function getFilesSync(dir, files = []) {
  readdirSync(dir, { withFileTypes: true }).forEach((entry) => {
    const entryPath = resolve(dir, entry.name);
    if (entry.isDirectory()) {
      getFilesSync(entryPath, files);
    } else {
      files.push(entryPath);
    }
  });
  return files;
}

function coherseToBuffer(input) {
  return Buffer.isBuffer(input) ? input : Buffer.from(input);
}

function cleanAndOpen(path) {
  if (existsSync(path)) {
    unlinkSync(path);
  }
  return createWriteStream(path, { flags: "w+" });
}

export default function progmemGenerator(options = {}) {
  const { outputPath, bytesPerLine = 20, indent = "  ", includes = ARDUINO_INCLUDES } = options;
  let viteConfig;

  return {
    name: 'progmem-generator',

    configResolved(resolvedConfig) {
      viteConfig = resolvedConfig;
    },

    closeBundle(_, bundle) {
      const fileInfo = [];
      const writeStream = cleanAndOpen(outputPath);

      const writeIncludes = () => {
        writeStream.write(includes);
      };

      const writeFile = (relativeFilePath, buffer) => {
        const variable = "ESP_REACT_DATA_" + fileInfo.length;
        const mimeType = mime.lookup(relativeFilePath);
        let size = 0;

        writeStream.write(`const uint8_t ${variable}[] PROGMEM = {`);
        
        const zipBuffer = zlib.gzipSync(buffer);
        zipBuffer.forEach((b) => {
          if (!(size % bytesPerLine)) {
            writeStream.write("\n");
            writeStream.write(indent);
          }
          writeStream.write(`0x${("00" + b.toString(16).toUpperCase()).substr(-2)},`);
          size++;
        });

        if (size % bytesPerLine) {
          writeStream.write("\n");
        }

        writeStream.write("};\n\n");

        fileInfo.push({
          uri: '/' + relativeFilePath.replace(new RegExp(`\\${sep}`, 'g'), '/'),
          mimeType,
          variable,
          size
        });
      };

      const writeFiles = () => {
        // process static files
        const buildPath = viteConfig.build.outDir;
        for (const filePath of getFilesSync(buildPath)) {
          const readStream = readFileSync(filePath);
          const relativeFilePath = relative(buildPath, filePath);
          writeFile(relativeFilePath, readStream);
        }
        // process assets
        //const { output } = viteConfig;
        //console.log(output);
        /*
        Object.keys(output).forEach((fileName) => {
          const asset = output[fileName];
          if (asset.type === 'asset') {
            writeFile(fileName, coherseToBuffer(asset.source));
          }
        });
        */
      };
      // ${fileInfo.map((file) => `${indent.repeat(3)}handler("${file.uri}", "${file.mimeType}", ${file.variable}, ${file.size});`).join('\n')}
      const generateWWWClass = () => {
        return `typedef std::function<void(const String& uri, const String& contentType, const uint8_t * content)> RouteRegistrationHandler;

class WWWData {
${indent}public:
${indent.repeat(2)}static void registerRoutes(RouteRegistrationHandler handler) {
${fileInfo.map((file) => `${indent.repeat(3)}handler("${file.uri}", "${file.mimeType}", ${file.variable});`).join('\n')}
${indent.repeat(2)}}
};
`;
      };

      const writeWWWClass = () => {
        writeStream.write(generateWWWClass());
      };

      writeIncludes();
      writeFiles();
      writeWWWClass();

      writeStream.end();
    }
  };
}
