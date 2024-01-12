/* @refresh reload */
import { createSignal, lazy } from 'solid-js';
import { render } from 'solid-js/web';
import { Router, Route } from '@solidjs/router';

import './index.css';
import App from './app';

const SignUp = lazy(() => import('./sign-up'));

const [user, setUser] = createSignal({});
const root = document.getElementById('root');

render(
	() => (
		<Router>
			<Route path="/" component={() => (<App user={user()} />)} />
			<Route path="/sign-up" component={() => (<SignUp setUser={setUser} />)} />
		</Router>
	),
	root
);
