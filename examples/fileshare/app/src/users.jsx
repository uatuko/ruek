import {
	createEffect,
	createSignal,
	onMount,
	For,
	Show
} from 'solid-js';
import { useNavigate } from '@solidjs/router';
import { IoHome } from 'solid-icons/io';

import Busy from './busy';

import './users.css';

function Users() {
	const nav = useNavigate();

	const [busy, setBusy] = createSignal(false);
	const [token, setToken] = createSignal('');
	const [users, setUsers] = createSignal([]);

	async function fetchUsers(token) {
		setBusy(true);
		const resp = await fetch(`http://localhost:3000/v1/users?pagination_token=${token}`);

		const data = await resp.json();
		setToken(data.pagination_token);

		setUsers((prev) => (
			[
				...prev,
				...data.users,
			]
		));

		setBusy(false);
	}

	function onScroll() {
		if (window.innerHeight + window.scrollY >= document.body.offsetHeight - 1) {
			const t = token();
			if (t) {
				fetchUsers(token());
			}
		}
	}

	createEffect(() => {
		window.addEventListener('scroll', onScroll);
	});

	onMount(async () => {
		await fetchUsers(token());
	});

	return (
		<div class="users">
			<div class="toolbar">
				<button onClick={() => {nav('/');}}><IoHome /></button>
			</div>
			<div class="list">
				<div class="row header">
					<div class="cell">Id</div>
					<div class="cell">Name</div>
				</div>
				<For each={users()}>
					{
						(user) => (
							<div class="row">
								<div class="cell">{user.id}</div>
								<div class="cell">{user.name}</div>
							</div>
						)
					}
				</For>
				<Show when={busy()}><Busy /></Show>
			</div>
		</div>
	);
}

export default Users;
