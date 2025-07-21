export * from './binding';
export * from './endpoints';
export * from './route';
export * from './submit';
export * from './time';
export * from './useRest';
export * from './useWs';
export * from './props';

// Simple debounce utility function
export function debounce<T extends (...args: any[]) => any>(
  func: T,
  delay: number
): (...args: Parameters<T>) => void {
  let timeoutId: NodeJS.Timeout;
  return (...args: Parameters<T>) => {
    clearTimeout(timeoutId);
    timeoutId = setTimeout(() => func(...args), delay);
  };
}
