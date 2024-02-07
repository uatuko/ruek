import { createEffect, createResource, Show } from 'solid-js';
import { useNavigate, useParams } from '@solidjs/router';
import { IoCloseCircleOutline, IoDocumentOutline } from 'solid-icons/io';

import NewFile from './new-file';

import './file.css';

function FileDetails(props) {
	function onClose() {
		props?.onClose && props.onClose();
	}

	return (
		<div class="file-details">
			<div class="toolbar">
				<button onClick={onClose}><IoCloseCircleOutline /></button>
			</div>

			<div class="flex">
				<div class="icon">
					<IoDocumentOutline />
				</div>
				<div class="detail">
					<div class="group">
						<div class="label">Id</div>
						<div>{props.data.id}</div>
					</div>
					<div class="group">
						<div class="label">Name</div>
						<div>{props.data.name}</div>
					</div>
					<div class="group">
						<div class="label">Access</div>
						<div>{props.data.role}</div>
					</div>
				</div>
			</div>
		</div>
	);
}

function File(props) {
	const params = useParams();
	const nav = useNavigate();

	const [file, {mutate : setFile}] = createResource(
		() => [params.id, props.user.id],
		async ([fileId, userId]) => {
			if (fileId === ':new') {
				return {};
			}

			const resp = await fetch(`http://localhost:3000/v1/files/${fileId}`, {
				headers : {
					'user-id': userId,
				},
			});

			return resp.json();
		}
	);

	createEffect(() => {
		if (!props.user.id) {
			nav('/sign-up', { replace: true });
		}
	});

	function onClose() {
		nav('/');
	}

	function onNewFile(data) {
		setFile(data);
	}

	return (
		<Show
			when={file()?.id}
			fallback={() => (<NewFile user={props.user} onCancel={onClose} onSuccess={onNewFile} />)}
		>
			<Show
				when={!file.loading}
				fallback={() => (<span>Loading...</span>)}
			>
				<FileDetails data={file()} onClose={onClose} />
			</Show>
		</Show>
	);
}

export default File;
