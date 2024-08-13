
import { FC, useContext, useState, useEffect, useCallback } from 'react';

import {
  Button, IconButton, Table, TableBody, TableCell, TableFooter, TableHead, TableRow
} from '@mui/material';
//import SaveIcon from '@mui/icons-material/Save';
import DeleteIcon from '@mui/icons-material/Delete';
import AddIcon from '@mui/icons-material/Add';
import EditIcon from '@mui/icons-material/Edit';
import SyncOutlinedIcon from '@mui/icons-material/SyncOutlined';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';

import * as RemotesApi from "./api";
import { WEB_SOCKET_ROOT } from '../api/endpoints';
import { Remote, RemoteSettings, RemoteState } from './types';
import { FormLoader, SectionContent } from '../components';
import { createRemoteValidator } from './validators';
import { useRest, useWs } from '../utils';
import { AuthenticatedContext } from '../contexts/authentication';

import RemoteForm from './RemoteForm';
import PairingDialog from './Pairingdialog';
//import { Description } from '@mui/icons-material';

export const REMOTE_STATE_WEBSOCKET_URL = WEB_SOCKET_ROOT + "remoteState";

function compareRemotes(a: Remote, b: Remote) {
  if (a.serial < b.serial) {
    return -1;
  }
  if (a.serial > b.serial) {
    return 1;
  }
  return 0;
}

const RemotesStateForm: FC = () => {
  const {
    loadData, saving, data, setData, saveData, errorMessage
  } = useRest<RemoteSettings>({ read: RemotesApi.readRemoteSettings, update: RemotesApi.updateRemoteSettings });

  const [remote, setRemote] = useState<Remote>();
  const [creating, setCreating] = useState<boolean>(false);
  const [pairingDialogOpen, setPairingDialogOpen] = useState(false);
  const [pairingError, setPairingError] = useState<string>();
  const [pairingSuccess, setPairingSuccess] = useState(false);
  const [remoteToDelete, setRemoteToDelete] = useState<Remote | null>(null);
  const [highlightedRemote, setHighlightedRemote] = useState<string | null>(null);

  const authenticatedContext = useContext(AuthenticatedContext);

  const { data: wsData, updateData: updateWsData } = useWs<RemoteState>(REMOTE_STATE_WEBSOCKET_URL);

  const handlePairingCancel = () => {
    updateWsData({ pairing: false });
    setPairingError("Pairing canceled by user");
  };

  const handlePairingClose = () => {
    setPairingDialogOpen(false);
    setPairingError(undefined);
    setPairingSuccess(false);
    loadData();
  };

  const status = useCallback(() => {
    if(wsData) {
        const { remote_button, remote_serial, remote_description, remote_updated_at } = wsData;

        if(remote_button && remote_button > 0 && remote_serial && remote_serial != "") {
            return (
                <div>
                    <p><b>Button:</b> {remote_button} <b>Serial:</b> {remote_serial} <b>Description</b>: {remote_description}</p>
                    <p>Updated At: {remote_updated_at}</p>
                </div>
            );
        }
    }
    return null;
  }, [wsData]);

  useEffect(() => {
    if (wsData && wsData.remote_button && wsData.remote_serial) {
      const matchingRemote = data?.remotes.find(
        //r => r.button === wsData.remote_button && r.serial === wsData.remote_serial
        r => r.id === wsData.remote_id
      );
      if (matchingRemote) {
        setHighlightedRemote(matchingRemote.id);
        setTimeout(() => setHighlightedRemote(null), 1000);
      }
    }
  }, [wsData, data]);

  useEffect(() => {
    if (wsData && !wsData.pairing) {
      if (wsData.pairing_error) {
        setPairingError(wsData.pairing_error);
      } else {
        setPairingSuccess(true);
      }
    }
  }, [wsData]);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const removeRemote = (toRemove: Remote) => {
        setRemoteToDelete(toRemove);
    };

    const confirmRemoveRemote = () => {
        if (remoteToDelete) {
          RemotesApi.removeRemote(remoteToDelete);
          authenticatedContext.refresh();
          loadData();
          setRemoteToDelete(null);
        }
      };

      const cancelRemoveRemote = () => {
        setRemoteToDelete(null);
      };

    const createRemote = () => {
      setCreating(true);
      setRemote({
        id: "",
        description: "",
        serial: "",
        button: 0
      });
    };

    const startPairing = () => {
        setPairingDialogOpen(true);
        setPairingError(undefined);
        setPairingSuccess(false);
        updateWsData({ pairing: true });
    };

    const editRemote = (toEdit: Remote) => {
      setCreating(false);
      setRemote({ ...toEdit });
    };

    const cancelEditingRemote = () => {
      setRemote(undefined);
    };

    const doneEditingRemote = () => {
        console.log("Creating: " + creating);
        console.log("Remote: " + remote);
      if (remote) {
        if(creating) {
            RemotesApi.createRemote(remote);
            setRemote(undefined);
            authenticatedContext.refresh();
            loadData();
        }
        else {
            RemotesApi.updateRemote(remote);
            setRemote(undefined)
            authenticatedContext.refresh();
            loadData();
        }
      }
    };

    return (
      <>
        <Table size="small">
          <TableHead>
            <TableRow>
              <TableCell>Description</TableCell>
              <TableCell>Button</TableCell>
              <TableCell>Serial</TableCell>
              <TableCell />
            </TableRow>
          </TableHead>
          <TableBody>
            {data.remotes.sort(compareRemotes).map((r) => (
              <TableRow 
                key={r.id}
                style={{
                    transition: 'background-color 1s',
                    backgroundColor: highlightedRemote === r.id ? '#90EE90' : 'white'
                  }}
              >
                <TableCell component="th" scope="row">
                  {r.description}
                </TableCell>
                <TableCell>{r.button}</TableCell>
                <TableCell>{r.serial}</TableCell>
                <TableCell align="right">
                  <IconButton size="small" aria-label="Delete" onClick={() => removeRemote(r)}>
                    <DeleteIcon />
                  </IconButton>
                  <IconButton size="small" aria-label="Edit" onClick={() => editRemote(r)}>
                    <EditIcon />
                  </IconButton>
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
          <TableFooter>
            <TableRow>
              <TableCell colSpan={3} />
              <TableCell align="right">
                <Button startIcon={<SyncOutlinedIcon />} variant="contained" color="secondary" onClick={startPairing} sx={{ m: 1 }}>
                  Pair
                </Button>
                <Button startIcon={<AddIcon />} variant="contained" color="secondary" onClick={createRemote}>
                  Add
                </Button>
              </TableCell>
            </TableRow>
          </TableFooter>
        </Table>
        <RemoteForm
          remote={remote}
          setRemote={setRemote}
          creating={creating}
          onDoneEditing={doneEditingRemote}
          onCancelEditing={cancelEditingRemote}
          validator={createRemoteValidator(data.remotes, creating)}
        />
        <PairingDialog
          open={pairingDialogOpen}
          onClose={handlePairingClose}
          onCancel={handlePairingCancel}
          pairingError={pairingError}
          pairingSuccess={pairingSuccess}
        />
        <Dialog
          open={!!remoteToDelete}
          onClose={cancelRemoveRemote}
          aria-labelledby="alert-dialog-title"
          aria-describedby="alert-dialog-description"
        >
          <DialogTitle id="alert-dialog-title">{"Confirm Remote Deletion"}</DialogTitle>
          <DialogContent>
            <DialogContentText id="alert-dialog-description">
              Are you sure you want to delete the remote "{remoteToDelete?.description}"?
            </DialogContentText>
          </DialogContent>
          <DialogActions>
            <Button onClick={cancelRemoveRemote}>Cancel</Button>
            <Button onClick={confirmRemoveRemote} autoFocus>
              Confirm
            </Button>
          </DialogActions>
        </Dialog>
      </>
    );
  };

  return (
    <>
        <SectionContent title='Last remote received' titleGutter>
            {status()}
        </SectionContent>
        <SectionContent title='Manage Remotes' titleGutter>
            {content()}
        </SectionContent>
    </>
  );
};

export default RemotesStateForm;

/*
        <ButtonRow mt={2}>
          <Button
            startIcon={<SaveIcon />}
            disabled={saving}
            variant="contained"
            color="primary"
            type="submit"
            onClick={onSubmit}
          >
            Save
          </Button>
        </ButtonRow>



*/