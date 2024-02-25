import { createEffect } from 'solid-js';
import { useNavigate } from '@solidjs/router';
import { IoAdd, IoPeopleSharp } from 'solid-icons/io';

import Files from './files';

import './app.css';

function App(props) {
	const nav = useNavigate();

	createEffect(() => {
		if (!props.user.id) {
			nav('/users', { replace: true });
		}
	});

	return (
		<div class="app">
			<div class="toolbar">
				<button onClick={() => {nav('/users');}}><IoPeopleSharp /></button>
				<button onClick={() => {nav('/files/:new');}}>
					<IoAdd />
					<span>New File</span>
				</button>
			</div>

			<Files user={props.user} />
		</div>
	);
}

export default App;
