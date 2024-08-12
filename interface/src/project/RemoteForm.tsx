import React, { FC, useState, useEffect } from 'react';
import Schema, { ValidateFieldsError } from 'async-validator';

import { Button, Dialog, DialogActions, DialogContent, DialogTitle } from '@mui/material';

import { Remote } from './types';
import { ValidatedTextField } from '../components';
import { validate } from '../validators';
import { updateValue } from '../utils';

interface RemoteFormProps {
  creating: boolean;
  validator: Schema;

  remote?: Remote;
  setRemote: React.Dispatch<React.SetStateAction<Remote | undefined>>;

  onDoneEditing: () => void;
  onCancelEditing: () => void;
}

const RemoteForm: FC<RemoteFormProps> = ({ creating, validator, remote, setRemote, onDoneEditing, onCancelEditing }) => {

  const updateFormValue = updateValue(setRemote);
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const open = !!remote;

  useEffect(() => {
    if (open) {
      setFieldErrors(undefined);
    }
  }, [open]);

  const validateAndDone = async () => {
    if (remote) {
      try {
        setFieldErrors(undefined);
        await validate(validator, remote);
        onDoneEditing();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    }
  };

  return (
    <Dialog onClose={onCancelEditing} aria-labelledby="remote-form-dialog-title" open={!!remote} fullWidth maxWidth="sm">
      {
        remote &&
        <>
          <DialogTitle id="remote-form-dialog-title">{creating ? 'Add' : 'Modify'} Remote</DialogTitle>
          <DialogContent dividers>
            <ValidatedTextField
              fieldErrors={fieldErrors}
              name="description"
              label="Description"
              fullWidth
              variant="outlined"
              value={remote.description}
              onChange={updateFormValue}
              margin="normal"
            />
            <ValidatedTextField
              fieldErrors={fieldErrors}
              name="serial"
              label="Serial"
              fullWidth
              variant="outlined"
              value={remote.serial}
              onChange={updateFormValue}
              margin="normal"
            />
            <ValidatedTextField
              fieldErrors={fieldErrors}
              name="button"
              label="Button"
              fullWidth
              variant="outlined"
              value={remote.button}
              onChange={updateFormValue}
              margin="normal"
              type="number"
            />
          </DialogContent>
          <DialogActions>
            <Button variant="contained" onClick={onCancelEditing} color="secondary">
              Cancel
            </Button>
            <Button
              variant="contained"
              onClick={validateAndDone}
              color="primary"
              autoFocus
            >
              Done
            </Button>
          </DialogActions>
        </>
      }
    </Dialog>
  );
};

export default RemoteForm;