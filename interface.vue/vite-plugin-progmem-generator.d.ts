declare module 'vite-plugin-progmem-generator' {
    interface ProgmemGeneratorOptions {
      outputPath: string;
      bytesPerLine?: number;
      indent?: string;
      includes?: string;
    }
  
    export default function progmemGenerator(options?: ProgmemGeneratorOptions): any;
  }
  