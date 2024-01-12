import { Show } from 'solid-js';
import { Navigate } from '@solidjs/router';
import { IoCogSharp } from 'solid-icons/io';
import { IoAdd } from 'solid-icons/io';

import Files from './files';

import './app.css';

function App(props) {
	return (
		<Show
			when={props.user.id}
			fallback={<Navigate href="/sign-up" />}
		>
			<div class="toolbar">
				<button><IoCogSharp /></button>
				<button><IoAdd /></button>
			</div>

			<Files />
		</Show>
	);
}

export default App;
