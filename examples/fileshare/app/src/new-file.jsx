import { createSignal, Show } from 'solid-js';
import { IoCloseCircleOutline } from 'solid-icons/io';

import Busy from './busy';

import './new-file.css';

function NewFile(props) {
	const [busy, setBusy] = createSignal(false);
	const [fileName, setFileName] = createSignal('');

	const createFile = async (e) => {
		e.preventDefault();

		setBusy(true);
		const resp = await fetch('http://localhost:3000/v1/files', {
			method: 'POST',
			headers: {
				'content-type': 'application/json',
				'user-id': props.user.id,
			},
			body: JSON.stringify({ name: fileName() }),
		});

		setBusy(false);

		if (props?.onSuccess) {
			props.onSuccess(await resp.json());
		}
	};

	const onCancel = () => {
		props?.onCancel && props.onCancel();
	};

	return (
		<div class="new-file">
			<div class="toolbar">
				<button onClick={onCancel}><IoCloseCircleOutline /></button>
			</div>

			<form disabled={busy()} onSubmit={createFile}>
				<h2>New file</h2>
				<div class="group">
					<label for="file-name">File name</label>
					<input
						type="text"
						id="file-name"
						name="file-name"
						autocomplete="off"
						required minLength={2}
						placeholder="e.g. filename.txt"
						disabled={busy()}
						value={fileName()}
						onChange={(e) => setFileName(e.target.value)}
					/>
				</div>

				<div class="action">
					<Show
						when={!busy()}
						fallback={<Busy />}
					>
						<input
							type="submit"
							value="Save"
						/>
					</Show>
				</div>
			</form>
		</div>
	);
}

export default NewFile;
