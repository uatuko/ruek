import { createSignal, Show } from 'solid-js';
import { useNavigate } from '@solidjs/router';

import Busy from './busy';

import './sign-up.css';

function SignUp(props) {
	const [busy, setBusy] = createSignal(false);
	const [name, setName] = createSignal('');
	const nav = useNavigate();

	const signup = async (e) => {
		e.preventDefault();

		setBusy(true);
		try {
			const resp = await fetch('http://localhost:3000/v1/users', {
				method : 'POST',
				headers : {
					'content-type': 'application/json',
				},
				body : JSON.stringify({ name: name() }),
			});

			props.setUser(await resp.json());
		} catch (err) {
			console.log(err);
			return;
		}

		setBusy(false);
		nav('/');
	};

	return (
		<form class="sign-up" disabled={busy()} onSubmit={signup}>
			<div class="group">
				<label for="name">Name</label>
				<input
					type="text"
					id="name"
					name="name"
					autocomplete="name"
					required minLength={2}
					placeholder="e.g. Jane Smith"
					disabled={busy()}
					value={name()}
					onChange={(e) => setName(e.target.value)}
				/>
			</div>

			<div class="action">
				<Show
					when={!busy()}
					fallback={<Busy />}
				>
					<input
						type="submit"
						value="Sign Up"
					/>
				</Show>
			</div>
		</form>
	);
}

export default SignUp;
