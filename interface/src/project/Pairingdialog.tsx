import React, { FC, useState, useEffect } from 'react';
import { 
  Dialog, 
  DialogContent, 
  DialogActions, 
  Button, 
  Typography, 
  CircularProgress,
  Box
} from '@mui/material';

interface PairingDialogProps {
  open: boolean;
  onClose: () => void;
  onCancel: () => void;
  pairingError?: string;
  pairingSuccess?: boolean;
}

const PairingDialog: FC<PairingDialogProps> = ({ open, onClose, onCancel, pairingError, pairingSuccess }) => {
  const [timeElapsed, setTimeElapsed] = useState(0);
  const [timedOut, setTimedOut] = useState(false);
  const [canceledByUser, setCanceledByUser] = useState(false);

  useEffect(() => {
    let interval: NodeJS.Timeout;
    if (open && !pairingError && !pairingSuccess && !canceledByUser) {
      interval = setInterval(() => {
        setTimeElapsed((prev) => {
          if (prev >= 120) {
            setTimedOut(true);
            clearInterval(interval);
            return prev;
          }
          return prev + 1;
        });
      }, 1000);
    }
    return () => {
      if (interval) clearInterval(interval);
    };
  }, [open, pairingError, pairingSuccess, canceledByUser]);

  useEffect(() => {
    if (!open) {
      setTimeElapsed(0);
      setTimedOut(false);
      setCanceledByUser(false);
    }
  }, [open]);

  const handleCancel = () => {
    setCanceledByUser(true);
    onCancel();
  };

  const getMessage = () => {
    if (pairingError) return pairingError;
    if (pairingSuccess) return "Pairing successful";
    if (timedOut) return "Pairing timed out";
    if (canceledByUser) return "Pairing canceled by user";
    return "Pairing, please press remote button. Press 'ESC' to cancel.";
  };

  const showLoading = open && !timedOut && !pairingError && !pairingSuccess && !canceledByUser;
  const showCloseButton = timedOut || pairingError || pairingSuccess || canceledByUser;

  return (
    <Dialog 
      open={open} 
      onClose={(event, reason) => {
        if (reason === 'escapeKeyDown') {
          handleCancel();
        }
      }}
      disableEscapeKeyDown={false}
      maxWidth="sm"
      fullWidth
    >
      <DialogContent>
        <Box display="flex" flexDirection="column" alignItems="center" justifyContent="center" minHeight="150px">
          {showLoading && <CircularProgress style={{ marginBottom: 16 }} />}
          <Typography variant="body1" align="center">
            {getMessage()}
          </Typography>
        </Box>
      </DialogContent>
      {showCloseButton && (
        <DialogActions>
          <Button onClick={onClose} color="primary">
            Close
          </Button>
        </DialogActions>
      )}
    </Dialog>
  );
};

export default PairingDialog;